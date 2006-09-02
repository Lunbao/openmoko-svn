#ifndef __GSMD_SELECT_H
#define __GSMD_SELECT_H

#include <common/linux_list.h>

#define GSMD_FD_READ	0x0001
#define GSMD_FD_WRITE	0x0002
#define GSMD_FD_EXCEPT	0x0004

struct gsmd_fd {
	struct llist_head list;
	int fd;				/* file descriptor */
	unsigned int when;
	int (*cb)(int fd, unsigned int what, void *data);
	void *data;			/* void * to pass to callback */
};

int gsmd_register_fd(struct gsmd_fd *ufd);
void gsmd_unregister_fd(struct gsmd_fd *ufd);
int gsmd_select_main(void);

#endif
