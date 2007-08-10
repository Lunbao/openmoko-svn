/*
 *  OpenMoko Media Player
 *   http://openmoko.org/
 *
 *  Copyright (C) 2007 by the OpenMoko team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * @file guitools.h
 * Various helper functions to aid with GUI creation and handling
 */

#ifndef GUITOOLS_H
#define GUITOOLS_H

#include <gtk/gtk.h>

#include "main.h"

extern gchar *ui_image_path;

GdkPixbuf *pixbuf_new_from_file(const gchar* file_name);
GtkWidget *create_label(GtkWidget **label, gchar *font_info, gchar *color_desc, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale, gint max_char_count);
void container_add_image(GtkContainer *container, gchar *image_name);
void container_add_image_with_ref(GtkContainer *container, gchar *image_name, GtkWidget **image);

// These are for the main window's notebook used to switch UI pages
void omp_notebook_add_page_with_icon(GtkWidget *notebook, GtkWidget *child, const gchar *icon_name, int padding);


#endif