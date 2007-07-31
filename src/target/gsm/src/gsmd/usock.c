/* gsmd unix domain socket handling
 *
 * (C) 2006-2007 by OpenMoko, Inc.
 * Written by Harald Welte <laforge@openmoko.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "gsmd.h"

#include <gsmd/gsmd.h>
#include <gsmd/usock.h>
#include <gsmd/select.h>
#include <gsmd/atcmd.h>
#include <gsmd/usock.h>
#include <gsmd/talloc.h>
#include <gsmd/ts0707.h>

static void *__ucmd_ctx, *__gu_ctx;

struct gsmd_ucmd *ucmd_alloc(int extra_size)
{
	return talloc_size(__ucmd_ctx, 
			   sizeof(struct gsmd_ucmd) + extra_size);
}

void usock_cmd_enqueue(struct gsmd_ucmd *ucmd, struct gsmd_user *gu)
{
	DEBUGP("enqueueing usock cmd %p for user %p\n", ucmd, gu);

	/* add to per-user list of finished cmds */
	llist_add_tail(&ucmd->list, &gu->finished_ucmds);

	/* mark socket of user as we-want-to-write */
	gu->gfd.when |= GSMD_FD_WRITE;
}

/* callback for completed passthrough gsmd_atcmd's */
static int usock_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	int rlen = strlen(resp)+1;
	struct gsmd_ucmd *ucmd = ucmd_alloc(rlen);

	DEBUGP("entering(cmd=%p, gu=%p)\n", cmd, gu);

	if (!ucmd)
		return -ENOMEM;
	
	/* FIXME: pass error values back somehow */
	ucmd->hdr.version = GSMD_PROTO_VERSION;
	ucmd->hdr.msg_type = GSMD_MSG_PASSTHROUGH;
	ucmd->hdr.msg_subtype = GSMD_PASSTHROUGH_RESP;
	ucmd->hdr.len = strlen(resp)+1;
	ucmd->hdr.id = cmd->id;
	memcpy(ucmd->buf, resp, ucmd->hdr.len);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

typedef int usock_msg_handler(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, int len);

static int usock_rcv_passthrough(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, int len)
{
	struct gsmd_atcmd *cmd;
	cmd = atcmd_fill((char *)gph+sizeof(*gph), gph->len, &usock_cmd_cb, gu, gph->id);
	if (!cmd)
		return -ENOMEM;

	DEBUGP("submitting cmd=%p, gu=%p\n", cmd, gu);

	return atcmd_submit(gu->gsmd, cmd);
}

static int usock_rcv_event(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, int len)
{
	u_int32_t *evtmask = (u_int32_t *) ((char *)gph + sizeof(*gph), gph->id);

	if (len < sizeof(*gph) + sizeof(u_int32_t))
		return -EINVAL;

	if (gph->msg_subtype != GSMD_EVT_SUBSCRIPTIONS)
		return -EINVAL;

	gu->subscriptions = *evtmask;
}

static int usock_rcv_voicecall(struct gsmd_user *gu, struct gsmd_msg_hdr *gph,
				int len)
{
	struct gsmd_atcmd *cmd = NULL;
	struct gsmd_addr *ga;
	struct gsmd_dtmf *gd;
	int atcmd_len;

	switch (gph->msg_subtype) {
	case GSMD_VOICECALL_DIAL:
		if (len < sizeof(*gph) + sizeof(*ga))
			return -EINVAL;
		ga = (struct gsmd_addr *) ((void *)gph + sizeof(*gph));
		ga->number[GSMD_ADDR_MAXLEN] = '\0';
		cmd = atcmd_fill("ATD", 5 + strlen(ga->number),
				 &usock_cmd_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "ATD%s;", ga->number);
		/* FIXME: number type! */
		break;
	case GSMD_VOICECALL_HANGUP:
		/* ATH0 is not supported by QC, we hope ATH is supported by everone */
		cmd = atcmd_fill("ATH", 5, &usock_cmd_cb, gu, gph->id);
		break;
	case GSMD_VOICECALL_ANSWER:
		cmd = atcmd_fill("ATA", 4, &usock_cmd_cb, gu, gph->id);
		break;
	case GSMD_VOICECALL_DTMF:
		if (len < sizeof(*gph) + sizeof(*gd))
			return -EINVAL;

		gd = (struct gsmd_dtmf *) ((void *)gph + sizeof(*gph));
		if (len < sizeof(*gph) + sizeof(*gd) + gd->len)
			return -EINVAL;

		/* FIXME: we don't yet support DTMF of multiple digits */
		if (gd->len != 1)
			return -EINVAL;

		atcmd_len = 1 + strlen("AT+VTS=") + (gd->len * 2);
		cmd = atcmd_fill("AT+VTS=", atcmd_len, &usock_cmd_cb,
				 gu, gph->id);
		if (!cmd)
			return -ENOMEM;

		sprintf(cmd->buf, "AT+VTS=%c;", gd->dtmf[0]);
		break;
	default:
		return -EINVAL;
	}

	if (cmd)
		return atcmd_submit(gu->gsmd, cmd);
	else
		return 0;
}

static int null_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	gsmd_log(GSMD_DEBUG, "null cmd cb\n");
	return 0;
}

/* PIN command callback. Gets called for response to AT+CPIN cmcd */
static int pin_cmd_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	gsmd_log(GSMD_DEBUG, "pin cmd cb\n");

	/* We need to verify if there is some error */
	switch (cmd->ret) {
	case 0:
		break;
	case GSM0707_CME_INCORRECT_PASSWORD:
		/* prompt for pin again */
		break;
	default:	
		/* something went wrong */
		break;
	}
	return 0;
}

static int usock_rcv_pin(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, 
			 int len)
{
	struct gsmd_pin *gp = (struct gsmd_pin *) ((void *)gph + sizeof(*gph));
	struct gsmd_atcmd *cmd;

	if (gph->len < sizeof(*gp) || len < sizeof(*gp)+sizeof(*gph))
		return -EINVAL;

	gsmd_log(GSMD_DEBUG, "pin type=%u, pin='%s', newpin='%s'\n",
		 gp->type, gp->pin, gp->newpin);

	switch (gph->msg_subtype) {
	case GSMD_PIN_INPUT:
		/* FIXME */
		break;
	default:
		gsmd_log(GSMD_ERROR, "unknown pin type %u\n",
			 gph->msg_subtype);
		return -EINVAL;
	}

	cmd = atcmd_fill("AT+CPIN=\"", 9+GSMD_PIN_MAXLEN+3+GSMD_PIN_MAXLEN+2,
			 &pin_cmd_cb, gu, 0);
	if (!cmd)
		return -ENOMEM;

	strcat(cmd->buf, gp->pin);

	switch (gp->type) {
	case GSMD_PIN_SIM_PUK:
	case GSMD_PIN_SIM_PUK2:
		strcat(cmd->buf, "\",\"");
		strcat(cmd->buf, gp->newpin);
		break;
	default:
		break;
	}

	strcat(cmd->buf, "\"");

	return atcmd_submit(gu->gsmd, cmd);
}

static int usock_rcv_phone(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, 
			   int len)
{
	struct gsmd_atcmd *cmd;

	switch (gph->msg_subtype) {
	case GSMD_PHONE_POWERUP:
		cmd = atcmd_fill("AT+CFUN=1", 9+1,
				 &null_cmd_cb, gu, 0);
		gu->gsmd->dev_state.on = 1;
		break;

	case GSMD_PHONE_POWERDOWN:
		cmd = atcmd_fill("AT+CFUN=0", 9+1,
				 &null_cmd_cb, gu, 0);
		gu->gsmd->dev_state.on = 0;
		break;
	default:
		return -EINVAL;
	}
	if (!cmd)
		return -ENOMEM;

	return atcmd_submit(gu->gsmd, cmd);
}

static int network_vmail_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_voicemail *vmail;
	struct gsmd_ucmd *ucmd;
	char *comma;

	DEBUGP("entering(cmd=%p, gu=%p)\n", cmd, gu);

	ucmd = ucmd_alloc(sizeof(*vmail));
	if (!ucmd)
		return -ENOMEM;
	
	/* FIXME: pass error values back somehow */
	ucmd->hdr.version = GSMD_PROTO_VERSION;
	ucmd->hdr.msg_type = GSMD_MSG_NETWORK;
	ucmd->hdr.len = sizeof(*vmail);
	ucmd->hdr.id = cmd->id;

	if (cmd->buf[7] == '=') {
		/* response to set command */
		ucmd->hdr.msg_subtype = GSMD_NETWORK_VMAIL_SET;
		/* FIXME: */
		return 0;
	} else {
		/* response to get command */
		char *tok = strtok(resp, ",");
		if (!tok)
			goto out_free_einval;
		ucmd->hdr.msg_subtype = GSMD_NETWORK_VMAIL_GET;
		vmail->enable = atoi(tok);

		tok = strtok(NULL, ",");
		if (!tok)
			goto out_free_einval;
		strncpy(vmail->addr.number, tok, GSMD_ADDR_MAXLEN);
		vmail->addr.number[GSMD_ADDR_MAXLEN] = '\0';

		tok = strtok(NULL, ",");
		if (!tok)
			goto out_free_einval;
		vmail->addr.type = atoi(tok);
	}

	usock_cmd_enqueue(ucmd, gu);

	return 0;

out_free_einval:
	gsmd_log(GSMD_ERROR, "can't understand voicemail response\n");
	talloc_free(ucmd);
	return -EINVAL;
}

static struct gsmd_ucmd *gsmd_ucmd_fill(int len, u_int8_t msg_type, u_int8_t msg_subtype,
					u_int16_t id)
{
	struct gsmd_ucmd *ucmd;

	ucmd = ucmd_alloc(len);
	if (!ucmd)
		return NULL;
	
	ucmd->hdr.version = GSMD_PROTO_VERSION;
	ucmd->hdr.msg_type = msg_type;
	ucmd->hdr.msg_subtype = msg_subtype;
	ucmd->hdr.len = len;
	ucmd->hdr.id = id;

	return ucmd;
}

static int network_sigq_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_signal_quality *gsq;
	struct gsmd_ucmd *ucmd;
	char *comma;
	
	ucmd = gsmd_ucmd_fill(sizeof(*gsq), GSMD_MSG_NETWORK,
			      GSMD_NETWORK_SIGQ_GET, 0);
	if (!ucmd)
		return -ENOMEM;
	
	gsq = (struct gsmd_signal_quality *) ucmd->buf;
	gsq->rssi = atoi(resp);
	comma = strchr(resp, ',');
	if (!comma) {
		talloc_free(ucmd);
		return -EIO;
	}
	gsq->ber = atoi(comma+1);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

#define GSMD_OPER_MAXLEN	16
static int network_oper_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_ucmd *ucmd;
	char *comma, *opname;
	
	ucmd = gsmd_ucmd_fill(GSMD_OPER_MAXLEN+1, GSMD_MSG_NETWORK,
			      GSMD_NETWORK_OPER_GET, 0);
	if (!ucmd)
		return -ENOMEM;

	/* Format: <mode>[, <format>, <oper>] */
	comma = strchr(resp, ',');
	if (!comma)
		goto out_err;

	if (atoi(comma+1) != 0) {
		gsmd_log(GSMD_NOTICE, "COPS format !=0 not supported yet!\n");
		goto out_err;
	}
	comma = strchr(resp, ',');
	if (!comma || *(comma+1) != '"')
		goto out_err;
	opname = comma+2;

	memcpy(ucmd->buf, opname, strlen(opname-1));
	ucmd->buf[strlen(opname)] = '\0';

	usock_cmd_enqueue(ucmd, gu);

	return 0;

out_err:
	talloc_free(ucmd);
	return -EIO;
}

static int usock_rcv_network(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, 
			     int len)
{
	struct gsmd_atcmd *cmd;
	struct gsmd_voicemail *vmail = (struct gsmd_voicemail *) gph->data;

	switch (gph->msg_subtype) {
	case GSMD_NETWORK_REGISTER:
		cmd = atcmd_fill("AT+COPS=0", 9+1,
				 &null_cmd_cb, gu, 0);
		break;
	case GSMD_NETWORK_VMAIL_GET:
		cmd = atcmd_fill("AT+CSVM?", 8+1, &network_vmail_cb, gu, 0);
		break;
	case GSMD_NETWORK_VMAIL_SET:
		cmd = atcmd_fill("AT+CSVM=", 8+1, &network_vmail_cb, gu, 0);
		break;
	case GSMD_NETWORK_SIGQ_GET:
		cmd = atcmd_fill("AT+CSQ", 6+1, &network_sigq_cb, gu, 0);
		break;
	case GSMD_NETWORK_OPER_GET:
		cmd = atcmd_fill("AT+COPS?", 8+1, &network_oper_cb, gu, 0);
		break;
	default:
		return -EINVAL;
	}
	if (!cmd)
		return -ENOMEM;

	return atcmd_submit(gu->gsmd, cmd);
}

static int sms_list_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	

	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_SMS,
			      GSMD_SMS_LIST, 0);
	if (!ucmd)
		return -ENOMEM;

	/* FIXME: implementation */
	strcpy(ucmd->buf, resp);
	
	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int sms_read_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	

	/* FIXME: implementation */

	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_SMS,
			      GSMD_SMS_READ, 0);
	if (!ucmd)
		return -ENOMEM;

	strcpy(ucmd->buf, resp);	

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int sms_send_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	
	
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_SMS,
			      GSMD_SMS_SEND, 0);
	if (!ucmd)
		return -ENOMEM;
	
	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int sms_write_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	
	
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_SMS,
			      GSMD_SMS_WRITE, 0);
	if (!ucmd)
		return -ENOMEM;	

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int sms_delete_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	

	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_SMS,
			      GSMD_SMS_DELETE, 0);
	if (!ucmd)
		return -ENOMEM;	

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int usock_rcv_sms(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, 
			 int len)
{
	/* FIXME: TEXT mode support!!  */
	struct gsmd_atcmd *cmd = NULL;
	struct gsmd_sms_delete *gsd;
	struct gsmd_sms *gs;
	struct gsmd_sms_write *gsw;
	int *stat, *index;
	int atcmd_len;
	char buf[1024];
	
	switch (gph->msg_subtype) {
	case GSMD_SMS_LIST:
		/* FIXME: only support PDU mode!! */
		if(len < sizeof(*gph) + sizeof(int))
			return -EINVAL;
		stat = (int *) ((void *)gph + sizeof(*gph));	

		sprintf(buf, "%d", *stat);	
				
		atcmd_len = 1 + strlen("AT+CMGL=") + strlen(buf);
		cmd = atcmd_fill("AT+CMGL=", atcmd_len,
				 &sms_list_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CMGL=%s", buf);
		break;
	case GSMD_SMS_READ:
		/* FIXME: only support PDU mode!! */
		if(len < sizeof(*gph) + sizeof(int))
			return -EINVAL;
		index = (int *) ((void *)gph + sizeof(*gph));

		sprintf(buf, "%d", *index);
		
		atcmd_len = 1 + strlen("AT+CMGR=") + strlen(buf);
		cmd = atcmd_fill("AT+CMGR=", atcmd_len,
				 &sms_read_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CMGR=%s", buf);
		break;
#if 0
	case GSMD_SMS_SEND:
		/* FIXME: only support PDU mode!! */
		if(len < sizeof(*gph) + sizeof(*gs))
			return -EINVAL;
		gs = (struct gsmd_sms *) ((void *)gph + sizeof(*gph));

		sprintf(buf, "%d", *index);
		
		atcmd_len = 1 + strlen("AT+CMGR=") + 1;
		cmd = atcmd_fill("AT+CMGR=", atcmd_len,
				 &sms_send_cb, gu, gph->id);
		if (!cmd)GSMD_SMS_WRITE
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CMGR=%d", index);
		break;
	case GSMD_SMS_WRITE:
		/* FIXME: only support PDU mode!! */
		if(len < sizeof(*gph) + sizeof(*gsw))
			return -EINVAL;
		&index = (int *) ((void *)gph + sizeof(*gph));
		
		atcmd_len = 1 + strlen("AT+CMGR=") + 1;
		cmd = atcmd_fill("AT+CMGR=", atcmd_len,
				 &sms_write_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CMGR=%d", index);
		break;
#endif
	case GSMD_SMS_DELETE:		
		if(len < sizeof(*gph) + sizeof(*gsd))
			return -EINVAL;
		gsd = (struct gsmd_sms_delete *) ((void *)gph + sizeof(*gph));
	    
		sprintf(buf, "%d,%d", gsd->index, gsd->delflg);
	
		atcmd_len = 1 + strlen("AT+CMGR=") + strlen(buf);
		cmd = atcmd_fill("AT+CMGD=", atcmd_len,
				 &sms_delete_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CMGD=%s", buf);		
		break;	
	default:
		return -EINVAL;
	}
		
	gsmd_log(GSMD_DEBUG, "%s\n", cmd->buf);
	if (cmd)
		return atcmd_submit(gu->gsmd, cmd);
	else
		return 0;
}

#if 0
static int phonebook_find_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;			
	
	/* FIXME: implementation */
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_FIND, 0);
	if (!ucmd)
		return -ENOMEM;	

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);
	return 0;
}

static int phonebook_read_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebook *gp;
	struct gsmd_ucmd *ucmd;
	char *fcomma, *lcomma;
	char *ptr;
	/* FIXME: We should check this case "When the entry is empty" */
	ucmd = gsmd_ucmd_fill(sizeof(*gp), GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_READ, 0);	

	if (!ucmd)
		return -ENOMEM;	
	gp = (struct gsmd_phonebook *) ucmd->buf;
	
	ptr = strchr(resp, ' ');	
	gp->index = atoi(ptr+1);		
	
	fcomma = strchr(resp, '"');
	lcomma = strchr(fcomma+1, '"');		
	strncpy(gp->numb, fcomma+1, (lcomma-fcomma-1));	
	gp->numb[(lcomma-fcomma)-1] = '\0';			
		
	gp->type = atoi(lcomma+2);		

	ptr = strrchr(resp, ',');
	fcomma = ptr+1;
	lcomma = strchr(fcomma+1, '"');
	strncpy(gp->text, fcomma+1, (lcomma-fcomma-1));
	gp->text[(lcomma-fcomma)-1] = '\0';
			
	usock_cmd_enqueue(ucmd, gu);
	
	return 0;
}

static int phonebook_readrg_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_ucmd *ucmd;	
	
	/* FIXME: implementation */
	
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_READRG, 0);
	if (!ucmd)
		return -ENOMEM;	

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int phonebook_write_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;	
	struct gsmd_ucmd *ucmd;	
		
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_WRITE, 0);
	if (!ucmd)
		return -ENOMEM;

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int phonebook_delete_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;		
	struct gsmd_ucmd *ucmd;		
		
	ucmd = gsmd_ucmd_fill(strlen(resp)+1, GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_DELETE, 0);
	if (!ucmd)
		return -ENOMEM;	

	strcpy(ucmd->buf, resp);

	usock_cmd_enqueue(ucmd, gu);

	return 0;
}

static int phonebook_support_cb(struct gsmd_atcmd *cmd, void *ctx, char *resp)
{
	struct gsmd_user *gu = ctx;
	struct gsmd_phonebook_support *gps;
	struct gsmd_ucmd *ucmd;
	char *fcomma, *lcomma;
	char *dash;
	
	ucmd = gsmd_ucmd_fill(sizeof(*gps), GSMD_MSG_PHONEBOOK,
			      GSMD_PHONEBOOK_GET_SUPPORT, 0);
	if (!ucmd)
		return -ENOMEM;
	
	gps = (struct gsmd_phonebook_support *) ucmd->buf;
		
	dash = strchr(resp, '-');
	if (!dash) {
		talloc_free(ucmd);
		return -EIO;
	}	
	gps->index = atoi(dash+1);

	fcomma = strchr(resp, ',');
	if (!fcomma) {
		talloc_free(ucmd);
		return -EIO;
	}
	gps->nlength = atoi(fcomma+1);
	
	lcomma = strrchr(resp, ',');
	if (!lcomma) {
		talloc_free(ucmd);
		return -EIO;
	}	
	gps->tlength = atoi(lcomma+1);	

	usock_cmd_enqueue(ucmd, gu);
	return 0;
}

static int usock_rcv_phonebook(struct gsmd_user *gu, struct gsmd_msg_hdr *gph, 
			 int len)
{	
	struct gsmd_atcmd *cmd = NULL;
	struct gsmd_phonebook_readrg *gpr;
	struct gsmd_phonebook *gp;
	struct gsmd_phonebook_find *gpf;
	int *index;
	int atcmd_len;
	char buf[1024];
	
	switch (gph->msg_subtype) {
	case GSMD_PHONEBOOK_FIND:
		if(len < sizeof(*gph) + sizeof(*gpf))
			return -EINVAL;
		gpf = (struct gsmd_phonebook_find *) ((void *)gph + sizeof(*gph));		
				
		atcmd_len = 1 + strlen("AT+CPBF=\"") + strlen(gpf->findtext) + strlen("\"");
		cmd = atcmd_fill("AT+CPBF=\"", atcmd_len,
				 &phonebook_find_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBF=\"%s\"", gpf->findtext);
		break;
	case GSMD_PHONEBOOK_READ:
		if(len < sizeof(*gph) + sizeof(int))
			return -EINVAL;
		index = (int *) ((void *)gph + sizeof(*gph));
		
		sprintf(buf, "%d", *index);		

		atcmd_len = 1 + strlen("AT+CPBR=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBR=", atcmd_len,
				 &phonebook_read_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBR=%d", *index);
		break;
	case GSMD_PHONEBOOK_READRG:		
		if(len < sizeof(*gph) + sizeof(*gpr))
			return -EINVAL;
		gpr = (struct gsmd_phonebook_readrg *) ((void *)gph + sizeof(*gph));

		sprintf(buf, "%d,%d", gpr->index1, gpr->index2);		

		atcmd_len = 1 + strlen("AT+CPBR=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBR=", atcmd_len,
				 &phonebook_readrg_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBR=%s", buf);
		break;
	case GSMD_PHONEBOOK_WRITE:
		if(len < sizeof(*gph) + sizeof(*gp))
			return -EINVAL;
		gp = (struct gsmd_phonebook *) ((void *)gph + sizeof(*gph));

		sprintf(buf, "%d,\"%s\",%d,\"%s\"", gp->index, gp->numb, gp->type, gp->text);
		
		atcmd_len = 1 + strlen("AT+CPBW=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBW=", atcmd_len,
				 &phonebook_write_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBW=%s", buf);
		break;
	case GSMD_PHONEBOOK_DELETE:
		if(len < sizeof(*gph) + sizeof(int))
			return -EINVAL;
		index = (int *) ((void *)gph + sizeof(*gph));
	    	
		sprintf(buf, "%d", *index);
		
		atcmd_len = 1 + strlen("AT+CPBW=") + strlen(buf);
		cmd = atcmd_fill("AT+CPBW=", atcmd_len,
				 &phonebook_delete_cb, gu, gph->id);
		if (!cmd)
			return -ENOMEM;
		sprintf(cmd->buf, "AT+CPBW=%s", buf);
		break;	
	case GSMD_PHONEBOOK_GET_SUPPORT:
		cmd = atcmd_fill("AT+CPBR=?", 9+1,
				 &phonebook_support_cb, gu, gph->id);
		break;
	default:
		return -EINVAL;
	}	

	if (cmd)
		return atcmd_submit(gu->gsmd, cmd);
	else
		return 0;
}
#endif

static usock_msg_handler *pcmd_type_handlers[__NUM_GSMD_MSGS] = {
	[GSMD_MSG_PASSTHROUGH]	= &usock_rcv_passthrough,
	[GSMD_MSG_EVENT]	= &usock_rcv_event,
	[GSMD_MSG_VOICECALL]	= &usock_rcv_voicecall,
	[GSMD_MSG_PIN]		= &usock_rcv_pin,
	[GSMD_MSG_PHONE]	= &usock_rcv_phone,
	[GSMD_MSG_NETWORK]	= &usock_rcv_network,
	[GSMD_MSG_SMS]		= &usock_rcv_sms,	
	//[GSMD_MSG_PHONEBOOK]	= &usock_rcv_phonebook,
};

static int usock_rcv_pcmd(struct gsmd_user *gu, char *buf, int len)
{
	struct gsmd_msg_hdr *gph = (struct gsmd_msg_hdr *)buf;
	usock_msg_handler *umh;

	if (gph->version != GSMD_PROTO_VERSION)
		return -EINVAL;

	if (gph->msg_type >= __NUM_GSMD_MSGS)
		return -EINVAL;
	
	umh = pcmd_type_handlers[gph->msg_type];
	if (!umh)
		return -EINVAL;

	return umh(gu, gph, len);
}

/* callback for read/write on client (libgsmd) socket */
static int gsmd_usock_user_cb(int fd, unsigned int what, void *data)
{
	struct gsmd_user *gu = data;

	/* FIXME: check some kind of backlog and limit it */

	if (what & GSMD_FD_READ) {
		char buf[1024];
		int rcvlen;
		/* read data from socket, determine what he wants */
		rcvlen = read(fd, buf, sizeof(buf));
		if (rcvlen == 0) {
			DEBUGP("EOF, this client has just vanished\n");
			/* EOF, this client has just vanished */
			gsmd_unregister_fd(&gu->gfd);
			close(fd);
			/* destroy whole user structure */
			llist_del(&gu->list);
			/* FIXME: delete busy ucmds from finished_ucmds */
			talloc_free(gu);
			return 0;
		} else if (rcvlen < 0)
			return rcvlen;
		else
			return usock_rcv_pcmd(gu, buf, rcvlen);
	}

	if (what & GSMD_FD_WRITE) {
		/* write data from pending replies to socket */
		struct gsmd_ucmd *ucmd, *uctmp;
		llist_for_each_entry_safe(ucmd, uctmp, &gu->finished_ucmds,
					  list) {
			int rc;

			rc = write(fd, &ucmd->hdr, sizeof(ucmd->hdr) + ucmd->hdr.len);
			if (rc < 0) {
				DEBUGP("write return %d\n", rc);
				return rc;
			}
			if (rc == 0) {
				DEBUGP("write returns zero!!\n");
				break;
			}
			if (rc != sizeof(ucmd->hdr) + ucmd->hdr.len) {
				DEBUGP("short write\n");
				break;
			}

			DEBUGP("successfully sent cmd %p to user %p, freeing\n", ucmd, gu);
			llist_del(&ucmd->list);
			talloc_free(ucmd);
		}
		if (llist_empty(&gu->finished_ucmds))
			gu->gfd.when &= ~GSMD_FD_WRITE;
	}

	return 0;
}

/* callback for read on master-listen-socket */
static int gsmd_usock_cb(int fd, unsigned int what, void *data)
{
	struct gsmd *g = data;
	struct gsmd_user *newuser;

	/* FIXME: implement this */
	if (what & GSMD_FD_READ) {
		/* new incoming connection */
		newuser = talloc(__gu_ctx, struct gsmd_user);
		if (!newuser)
			return -ENOMEM;
		
		newuser->gfd.fd = accept(fd, NULL, 0);
		if (newuser->gfd.fd < 0) {
			DEBUGP("error accepting incoming conn: `%s'\n",
				strerror(errno));
			talloc_free(newuser);
		}
		newuser->gfd.when = GSMD_FD_READ;
		newuser->gfd.data = newuser;
		newuser->gfd.cb = &gsmd_usock_user_cb;
		newuser->gsmd = g;
		newuser->subscriptions = 0xffffffff;
		INIT_LLIST_HEAD(&newuser->finished_ucmds);

		llist_add(&newuser->list, &g->users);
		gsmd_register_fd(&newuser->gfd);
	}

	return 0;
}

/* handling of socket with incoming client connections */
int usock_init(struct gsmd *g)
{
	struct sockaddr_un sun;
	int fd, rc;

	__ucmd_ctx = talloc_named_const(gsmd_tallocs, 1, "ucmd");
	__gu_ctx = talloc_named_const(gsmd_tallocs, 1, "gsmd_user");

	fd = socket(PF_UNIX, GSMD_UNIX_SOCKET_TYPE, 0);
	if (fd < 0)
		return fd;
	
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	memcpy(sun.sun_path, GSMD_UNIX_SOCKET, sizeof(GSMD_UNIX_SOCKET));

	rc = bind(fd, (struct sockaddr *)&sun, sizeof(sun));
	if (rc < 0) {
		close(fd);
		return rc;
	}

	rc = listen(fd, 10);
	if (rc < 0) {
		close(fd);
		return rc;
	}

	g->gfd_sock.fd = fd;
	g->gfd_sock.when = GSMD_FD_READ | GSMD_FD_EXCEPT;
	g->gfd_sock.data = g;
	g->gfd_sock.cb = &gsmd_usock_cb;

	return gsmd_register_fd(&g->gfd_sock);
}
