#ifndef __GSMD_UNSOLICITED_H
#define __GSMD_UNSOLICITED_H

#include "gsmd.h"

int unsolicited_parse(struct gsmd *g, const char *buf, int len, const char *param);

#endif
