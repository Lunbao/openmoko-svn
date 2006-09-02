#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include <gsmd/usock.h>
#include <gsmd/event.h>

#include "gsmd.h"
#include "usock.h"

static struct gsmd_ucmd *build_event(u_int8_t type, u_int8_t subtype, u_int8_t len)
{
	struct gsmd_ucmd *ucmd = malloc(sizeof(*ucmd)+len);

	if (!ucmd)
		return NULL;

	ucmd->hdr.version = GSMD_PROTO_VERSION;
	ucmd->hdr.msg_type = type;
	ucmd->hdr.msg_subtype = subtype;
	ucmd->hdr.len = len;

	return ucmd;
}

static struct gsmd_ucmd *ucmd_copy(const struct gsmd_ucmd *orig)
{
	int size = sizeof(*orig) + orig->hdr.len;
	struct gsmd_ucmd *copy = malloc(size);

	if (copy)
		memcpy(copy, orig, size);

	return copy;
}

static int usock_evt_send(struct gsmd *gsmd, struct gsmd_ucmd *ucmd, u_int32_t evt)
{
	struct gsmd_user *gu;
	int num_sent = 0;

	llist_for_each_entry(gu, &gsmd->users, list) {
		if (gu->subscriptions & (1 << evt)) {
			struct gsmd_ucmd *cpy = ucmd_copy(ucmd);
			llist_add_tail(&ucmd->list, &gu->finished_ucmds);
			num_sent++;
			ucmd = cpy;
			if (!ucmd) {
				fprintf(stderr, "can't allocate memory for copy of ucmd\n");
				return num_sent;
			}
		}
	}

	return num_sent;
}

static int ring_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	/* FIXME: generate ring event */
	struct gsmd_ucmd *ucmd = build_event(GSMD_MSG_EVENT, GSMD_EVT_IN_CALL,
					     sizeof(struct gsmd_evt_auxdata));	
	struct gsmd_evt_auxdata *aux;
	if (!ucmd)
		return -ENOMEM;

	aux = (struct gsmd_evt_auxdata *)ucmd->buf;

	aux->u.call.type = GSMD_CALL_UNSPEC;

	return usock_evt_send(gsmd, ucmd, GSMD_EVT_IN_CALL);
}

static int cring_parse(const char *buf, int len, const char *param, struct gsmd *gsmd)
{
	struct gsmd_ucmd *ucmd = build_event(GSMD_MSG_EVENT, GSMD_EVT_IN_CALL,
					     sizeof(struct gsmd_evt_auxdata));
	struct gsmd_evt_auxdata *aux;
	if (!ucmd)
		return -ENOMEM;

	aux = (struct gsmd_evt_auxdata *) ucmd->buf;

	if (!strcmp(param, "VOICE")) {
		/* incoming voice call */
		aux->u.call.type = GSMD_CALL_VOICE;
	} else if (!strcmp(param, "SYNC")) {
		aux->u.call.type = GSMD_CALL_DATA_SYNC;
	} else if (!strcmp(param, "REL ASYNC")) {
		aux->u.call.type = GSMD_CALL_DATA_REL_ASYNC;
	} else if (!strcmp(param, "REL SYNC")) {
		aux->u.call.type = GSMD_CALL_DATA_REL_SYNC;
	} else if (!strcmp(param, "FAX")) {
		aux->u.call.type = GSMD_CALL_FAX;
	} else if (!strncmp(param, "GPRS ", 5)) {
		/* FIXME: change event type to GPRS */
		free(ucmd);
		return 0;
	}
	/* FIXME: parse all the ALT* profiles, Chapter 6.11 */

	return usock_evt_send(gsmd, ucmd, GSMD_EVT_IN_CALL);
}

/* Chapter 7.2, network registration */
static int creg_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	const char *comma = strchr(param, ',');
	struct gsmd_ucmd *ucmd = build_event(GSMD_MSG_EVENT, GSMD_EVT_NETREG,
					     sizeof(struct gsmd_evt_auxdata));
	struct gsmd_evt_auxdata *aux;
	if (!ucmd)
		return -ENOMEM;
	aux = (struct gsmd_evt_auxdata *) ucmd->buf;

	aux->u.netreg.state = atoi(param);
	if (comma) {
		/* FIXME: we also have location area code and cell id to parse (hex) */
	}

	return usock_evt_send(gsmd, ucmd, GSMD_EVT_NETREG);
}

/* Chapter 7.11, call waiting */
static int ccwa_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	const char *number;
	u_int8_t type, class;

	/* FIXME: parse */
	return 0;
}

/* Chapter 7.14, unstructured supplementary service data */
static int cusd_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 7.15, advise of charge */
static int cccm_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 10.1.13, GPRS event reporting */
static int cgev_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 10.1.14, GPRS network registration status */
static int cgreg_parse(const char *buf, int len, const char *param,
		       struct gsmd *gsmd)
{
	/* FIXME: parse */
	return 0;
}

/* Chapter 7.6, calling line identification presentation */
static int clip_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	struct gsmd_ucmd *ucmd = build_event(GSMD_MSG_EVENT, GSMD_EVT_IN_CLIP,
					     sizeof(struct gsmd_evt_auxdata));
	struct gsmd_evt_auxdata *aux;
	const char *comma = strchr(param, ',');

	if (!ucmd)
		return -ENOMEM;
	
	aux = (struct gsmd_evt_auxdata *) ucmd->buf;

	if (!comma)
		return -EINVAL;
	
	if (comma - param > GSMD_ADDR_MAXLEN)
		return -EINVAL;

	memcpy(aux->u.clip.addr.number, param, comma-param);
	/* FIXME: parse of subaddr, etc. */

	return usock_evt_send(gsmd, ucmd, GSMD_EVT_IN_CLIP);
}

/* Chapter 7.9, calling line identification presentation */
static int colp_parse(const char *buf, int len, const char *param,
		      struct gsmd *gsmd)
{
	struct gsmd_ucmd *ucmd = build_event(GSMD_MSG_EVENT, GSMD_EVT_OUT_COLP,
					     sizeof(struct gsmd_evt_auxdata));
	struct gsmd_evt_auxdata *aux;
	const char *comma = strchr(param, ',');

	if (!ucmd)
		return -ENOMEM;
	
	aux = (struct gsmd_evt_auxdata *) ucmd->buf;

	if (!comma)
		return -EINVAL;
	
	if (comma - param > GSMD_ADDR_MAXLEN)
		return -EINVAL;

	memcpy(aux->u.colp.addr.number, param, comma-param);
	/* FIXME: parse of subaddr, etc. */

	return usock_evt_send(gsmd, ucmd, GSMD_EVT_OUT_COLP);
}

struct gsmd_unsolicit {
	const char *prefix;
	int (*parse)(const char *unsol, int len, const char *param, struct gsmd *gsmd);
};

static const struct gsmd_unsolicit gsm0707_unsolicit[] = {
	{ "RING",	&ring_parse },
	{ "+CRING", 	&cring_parse },
	{ "+CREG",	&creg_parse },
	{ "+CCWA",	&ccwa_parse },
	{ "+CUSD",	&cusd_parse },
	{ "+CCCM",	&cccm_parse },
	{ "+CGEV",	&cgev_parse },
	{ "+CGREG",	&cgreg_parse },
	{ "+CLIP",	&clip_parse },
	{ "+COLP",	&colp_parse },
	/*
	{ "+CKEV",	&ckev_parse },
	{ "+CDEV",	&cdev_parse },
	{ "+CIEV",	&ciev_parse },
	{ "+CLAV",	&clav_parse },
	{ "+CCWV",	&ccwv_parse },
	{ "+CLAV",	&clav_parse },
	{ "+CSSU",	&cssu_parse },
	*/
};

/* called by midlevel parser if a response seems unsolicited */
int unsolicited_parse(struct gsmd *g, const char *buf, int len, const char *param)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gsm0707_unsolicit); i++) {
		const char *colon;
		if (strncmp(buf, gsm0707_unsolicit[i].prefix,
			     strlen(gsm0707_unsolicit[i].prefix)))
			continue;
		
		colon = strchr(buf, ':') + 1;
		if (colon > buf+len)
			colon = NULL;

		return gsm0707_unsolicit[i].parse(buf, len, colon, g);
	}

	/* FIXME: call vendor-specific unsolicited code parser */
	return -EINVAL;
}

