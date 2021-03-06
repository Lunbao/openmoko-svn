/* vi: set sw=2: */
/*
 * Copyright (C) 2007 by OpenMoko, Inc.
 * Written by OpenedHand Ltd <info@openedhand.com>
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
 */
#ifndef __MOKO_TIME_H__
#define  __MOKO_TIME_H__

#include <time.h>
#include <glib-object.h>

typedef struct _MokoTime MokoTime ;

MokoTime* moko_time_new_today () ;
MokoTime* moko_time_from_timet (const time_t t, gboolean is_date) ;
MokoTime* moko_time_from_string (const gchar *iso_format_date) ;
void moko_time_free (MokoTime *time) ;
time_t moko_time_as_timet (const MokoTime *time) ;

const gchar* moko_time_as_ical_string (MokoTime *t) ;

/* code to help bindings */
#define MOKO_TYPE_TIME moko_time_get_type ()
GType moko_time_get_type (void);
MokoTime *moko_time_copy(const MokoTime *src );

#endif /*__MOKO_TIME_H__*/
