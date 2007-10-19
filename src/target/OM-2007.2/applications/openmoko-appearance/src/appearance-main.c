/*
 * OpenMoko Appearance - Change various appearance related settings
 *
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

#include <gtk/gtk.h>
#include "appearance.h"
#include "appearance-colors.h"

int
main (int argc, char *argv[])
{
  AppearanceData *data;
  GtkWidget *window, *notebook, *label, *colors_page, *background_page;

  /* initialise gtk+ */
  gtk_init (&argc, &argv);

  /* initialise our application data struct
   * gnew0 allocates the memory and initialises it with 0s */
  data = g_new0 (AppearanceData, 1);

  /* create our main window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Appearance");
  /* connect the "delete" event of the window to the main quit function
   * this causes the program to quit (main loop to exit) when the window is
   * closed */
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);

  /* create a notebook and set the tab position to the bottom of the window */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_BOTTOM);

  /* Add the notebook to the window
   * GtkWindow inherits from GtkContainer, so we can case it as a GtkContainer
   * (here using the inbuilt macro) and then use it in gtk_container_* functions */
  gtk_container_add (GTK_CONTAINER (window), notebook);

  /* create our "pages" */
  colors_page = colors_page_new (data);
  background_page = gtk_label_new ("No background options yet");

  /* add the pages to the notebook */
  /* colors page */
  label = gtk_label_new ("Colors");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), colors_page, label);
  /* OpenMoko style means we make the tabs as big as possible by setting
   * tab-fill to true */
  gtk_container_child_set (GTK_CONTAINER (notebook), colors_page, "tab-expand", TRUE, NULL);

  /* background page */
  label = gtk_label_new ("Background");
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), background_page, label);
  gtk_container_child_set (GTK_CONTAINER (notebook), background_page, "tab-expand", TRUE, NULL);

  /* display the window and all the widgets inside it */
  gtk_widget_show_all (window);

  /* start the main loop */
  gtk_main ();

  return 0;
}
