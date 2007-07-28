/*   openmoko-dialer-window-dialer.c
 *
 *  Authored by Tony Guan<tonyguan@fic-sh.com.cn>
 *
 *  Copyright (C) 2006 FIC Shanghai Lab
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date) [$Author: Tony Guan $]
 */

#include <libmokoui/moko-ui.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkvbox.h>

#include "common.h"
#include "contacts.h"
#include "dialer-main.h"
#include "dialer-window-dialer.h"
#include "dialer-window-history.h"
#include "dialer-window-outgoing.h"

#if 1
static void
cb_delete_button_clicked (GtkButton * button, MokoDialerData * appdata)
{
  //gchar *codesinput = NULL;
  
  moko_dialer_textview_delete (appdata->moko_dialer_text_view);

  /* skip the auto list for the moment as it is too slow 
  if (moko_dialer_autolist_has_selected (appdata->moko_dialer_autolist))
  {
    //first of all, we un-select the selection.
    moko_dialer_autolist_set_select (appdata->moko_dialer_autolist, -1);

    //fill the textview with ""
    moko_dialer_textview_fill_it (appdata->moko_dialer_text_view, "");
    //moko_dialer_textview_set_color(moko_dialer_textview);
  }
  codesinput =
    g_strdup (moko_dialer_textview_get_input (appdata->moko_dialer_text_view, FALSE));

  if (codesinput
      && (g_utf8_strlen (codesinput, -1) >= MOKO_DIALER_MIN_SENSATIVE_LEN))
  {
    moko_dialer_autolist_refresh_by_string (appdata->moko_dialer_autolist,
                                            codesinput, TRUE);

    moko_dialer_textview_set_color (appdata->moko_dialer_text_view);
  }
  else
  {
    moko_dialer_autolist_hide_all_tips (appdata->moko_dialer_autolist);
  }

  if (codesinput)
    g_free (codesinput);
    */

}
#else
 static void
 cb_delete_button_clicked (GtkButton * button, MokoDialerData * appdata)
 {

   moko_dialer_textview_delete (appdata->moko_dialer_text_view);
   g_print (moko_dialer_textview_get_input
                (appdata->moko_dialer_text_view, FALSE));
 
   if (moko_dialer_autolist_has_selected (appdata->moko_dialer_autolist))
   {
   //first of all, we un-select the selection.
     moko_dialer_autolist_set_select (appdata->moko_dialer_autolist, -1);
 
     //fill the textview with ""
     moko_dialer_textview_fill_it (appdata->moko_dialer_text_view, " ");
     //moko_dialer_textview_set_color(moko_dialer_textview);
   }
   else
   {
    moko_dialer_textview_delete (appdata->moko_dialer_text_view);
    //refresh the autolist,but do not automaticall fill the textview
    gchar *codesinput = 0;
    codesinput =
      g_strdup (moko_dialer_textview_get_input
                (appdata->moko_dialer_text_view, FALSE));
 
    if (codesinput)
    {
     DBG_MESSAGE ("input %s", codesinput);
      if (g_utf8_strlen (codesinput, -1) >= MOKO_DIALER_MIN_SENSATIVE_LEN)
      {
        moko_dialer_autolist_refresh_by_string (appdata->
                                                moko_dialer_autolist,
                                                codesinput, FALSE);
        moko_dialer_textview_set_color (appdata->moko_dialer_text_view);
      }
      else
        moko_dialer_autolist_hide_all_tips (appdata->moko_dialer_autolist);
      g_free (codesinput);
    }
    else
    {
      DBG_WARN ("No input now.");
    }
  }
  
 }
#endif
static void
cb_history_button_clicked (GtkButton * button, MokoDialerData * appdata)
{
  if (!appdata->window_history)
    window_history_init (appdata);

  gtk_widget_show_all (appdata->window_history);

}

static void
cb_dialer_button_clicked (GtkButton * button, MokoDialerData * appdata)
{
  gchar *number;
  number = moko_dialer_textview_get_input (appdata->moko_dialer_text_view, TRUE);
  if (!number)
  {
    /* show the history window if no number entered */
    gtk_widget_show_all (appdata->window_history);
    return;
  }
  window_outgoing_dial (appdata, number);
}



static void
on_dialer_autolist_user_selected (GtkWidget * widget, gpointer para_pointer,
                                  gpointer user_data)
{
  gchar *codesinput;
  gint lenstring = 0;
  gint leninput = 0;
  MokoDialerData *appdata = (MokoDialerData *) user_data;
  MokoDialerTextview *moko_dialer_text_view = appdata->moko_dialer_text_view;
  AutolistEntry *ready_contact = (AutolistEntry *) para_pointer;
  codesinput = moko_dialer_textview_get_input (moko_dialer_text_view, FALSE);
  if (ready_contact->entry->content)
    lenstring = g_utf8_strlen (ready_contact->entry->content, -1);
  else
    lenstring = 0;

  if (codesinput)
    leninput = g_utf8_strlen (codesinput, -1);
  else
    leninput = 0;
  if (lenstring > leninput)
  {

    moko_dialer_textview_fill_it (moko_dialer_text_view,
                                  &(ready_contact->entry->
                                    content[leninput]));

  }
  else
    moko_dialer_textview_fill_it (moko_dialer_text_view, "");
  g_free (codesinput);

}

static void
on_dialer_autolist_user_confirmed (GtkWidget * widget, gpointer para_pointer,
                                   gpointer user_data)
{
#if 10
  MokoDialerData *appdata = (MokoDialerData *) user_data;
  MokoDialerTextview *moko_dialer_text_view = appdata->moko_dialer_text_view;
  AutolistEntry *ready_contact = (AutolistEntry *) para_pointer;
  DBG_MESSAGE ("GOT THE MESSAGE OF confirmed:%s",
               ready_contact->entry->content);
  moko_dialer_textview_confirm_it (moko_dialer_text_view,
                                   ready_contact->entry->content);
  
#endif
}

static void
on_dialer_autolist_nomatch (GtkWidget * widget, gpointer user_data)
{

  MokoDialerData *appdata = (MokoDialerData *) user_data;
  MokoDialerTextview *moko_dialer_text_view = appdata->moko_dialer_text_view;

  DBG_MESSAGE ("GOT THE MESSAGE OF no match");
  moko_dialer_textview_fill_it (moko_dialer_text_view, "");

}


static void
on_dialer_panel_user_input (GtkWidget * widget, gchar parac,
                            gpointer user_data)
{
  char input[2];
  input[0] = parac;
  input[1] = 0;
  //gchar *codesinput = NULL;

  MokoDialerData *appdata = (MokoDialerData *) user_data;
  MokoDialerTextview *moko_dialer_text_view = appdata->moko_dialer_text_view;


  moko_dialer_textview_insert (moko_dialer_text_view, input);
#if 0
  codesinput =
    g_strdup (moko_dialer_textview_get_input (moko_dialer_text_view, FALSE));

  if (codesinput
      && (g_utf8_strlen (codesinput, -1) >= MOKO_DIALER_MIN_SENSATIVE_LEN))
  {
    moko_dialer_autolist_refresh_by_string (appdata->moko_dialer_autolist,
                                            codesinput, TRUE);
  }
  else
  {
    moko_dialer_autolist_hide_all_tips (appdata->moko_dialer_autolist);
  }

  if (codesinput)
    g_free (codesinput);
#endif
}

static void
on_dialer_panel_user_hold (GtkWidget * widget, gchar parac,
                           gpointer user_data)
{
  g_print ("on_dialer_panel_user_hold:%c\n", parac);
}

static void
on_window_dialer_hide (GtkWidget * widget, MokoDialerData * appdata)
{
  appdata->window_present = 0;
}

static void
on_window_dialer_show (GtkWidget * widget, MokoDialerData * appdata)
{
  DBG_ENTER ();
  appdata->window_present = widget;
  DBG_LEAVE ();
}

gint
window_dialer_init (MokoDialerData * p_dialer_data)
{

  if (!p_dialer_data->window_dialer)
  {
    GdkColor color;
    gdk_color_parse ("black", &color);

    GtkWidget *vbox = NULL;
    GtkWidget *window = moko_finger_window_new ();
    GtkMenu *appmenu = GTK_MENU (gtk_menu_new ());
    GtkMenuItem *hideitem =
      GTK_MENU_ITEM (gtk_menu_item_new_with_label ("Close"));
    g_signal_connect_swapped (G_OBJECT (hideitem), "activate",
                      G_CALLBACK (gtk_widget_hide), window);
    gtk_menu_shell_append (GTK_MENU_SHELL (appmenu), GTK_WIDGET (hideitem));


    moko_finger_window_set_application_menu (MOKO_FINGER_WINDOW (window),
                                             appmenu);

    g_signal_connect (G_OBJECT (window), "delete_event",
                      G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (G_OBJECT (window), "show",
                      G_CALLBACK (on_window_dialer_show), p_dialer_data);
    g_signal_connect (G_OBJECT (window), "hide",
                      G_CALLBACK (on_window_dialer_hide), p_dialer_data);



    /* contents */
    vbox = gtk_vbox_new (FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new (FALSE, 10);


    GtkWidget *eventbox1 = gtk_event_box_new ();
    gtk_widget_set_name (eventbox1, "gtkeventbox-black");

    GtkWidget *autolist = moko_dialer_autolist_new ();
    moko_dialer_autolist_set_data (MOKO_DIALER_AUTOLIST (autolist),
                                   &(p_dialer_data->g_contactlist));
    p_dialer_data->moko_dialer_autolist = MOKO_DIALER_AUTOLIST (autolist);

    gtk_container_add (GTK_CONTAINER (eventbox1), autolist);
    gtk_box_pack_start (GTK_BOX (vbox), eventbox1, FALSE, FALSE, 0);

    gtk_widget_modify_bg (eventbox1, GTK_STATE_NORMAL, &color);

    g_signal_connect (GTK_OBJECT (autolist), "user_selected",
                      G_CALLBACK (on_dialer_autolist_user_selected),
                      p_dialer_data);


    g_signal_connect (GTK_OBJECT (autolist), "user_confirmed",
                      G_CALLBACK (on_dialer_autolist_user_confirmed),
                      p_dialer_data);

    g_signal_connect (GTK_OBJECT (autolist), "autolist_nomatch",
                      G_CALLBACK (on_dialer_autolist_nomatch), p_dialer_data);





    eventbox1 = gtk_event_box_new ();
    gtk_widget_set_name (eventbox1, "gtkeventbox-black");
    gtk_widget_modify_bg (eventbox1, GTK_STATE_NORMAL, &color);

    GtkWidget *mokotextview = moko_dialer_textview_new ();
    p_dialer_data->moko_dialer_text_view =
      MOKO_DIALER_TEXTVIEW (mokotextview);

    gtk_container_add (GTK_CONTAINER (eventbox1), mokotextview);
    gtk_box_pack_start (GTK_BOX (vbox), eventbox1, FALSE, FALSE, 0);

    GtkWidget *mokodialerpanel = moko_dialer_panel_new ();

    g_signal_connect (GTK_OBJECT (mokodialerpanel), "user_input",
                      G_CALLBACK (on_dialer_panel_user_input), p_dialer_data);


    g_signal_connect (GTK_OBJECT (mokodialerpanel), "user_hold",
                      G_CALLBACK (on_dialer_panel_user_hold), p_dialer_data);

    gtk_box_pack_start (GTK_BOX (hbox), mokodialerpanel, TRUE, TRUE, 5);



    //the buttons

    GtkStockItem stock_item;
    GtkWidget *vbox2 = gtk_vbox_new (FALSE, 10);

    GtkWidget *button_vbox, *alignment;
    GtkWidget *icon, *label;
    GtkWidget *button;

    /* delete button */
    button = gtk_button_new ();
    button_vbox = gtk_vbox_new (FALSE, 0);
    icon = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
    label = gtk_label_new ("Delete");
    gtk_box_pack_start (GTK_BOX (button_vbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (button_vbox), label, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (cb_delete_button_clicked), p_dialer_data);
    gtk_container_add (GTK_CONTAINER (button), button_vbox);
    gtk_widget_set_name (button, "mokofingerbutton-orange");

    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);


    /* history button */
    button = gtk_button_new ();
    button_vbox = gtk_vbox_new (FALSE, 0);
    gtk_stock_lookup (MOKO_STOCK_CALL_HISTORY, &stock_item);
    icon = gtk_image_new_from_stock (MOKO_STOCK_CALL_HISTORY, GTK_ICON_SIZE_BUTTON);
    label = gtk_label_new (stock_item.label);
    gtk_box_pack_start (button_vbox, icon, FALSE, FALSE, 0);
    gtk_box_pack_start (button_vbox, label, FALSE, FALSE, 0);
    gtk_container_add (GTK_CONTAINER (button), button_vbox);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (cb_history_button_clicked), p_dialer_data);
    gtk_widget_set_name (button, "mokofingerbutton-orange");
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);


    /* dial button */
    button = gtk_button_new ();
    button_vbox = gtk_vbox_new (FALSE, 0);
    alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (alignment), button_vbox);
    gtk_stock_lookup (MOKO_STOCK_CALL_DIAL, &stock_item);
    icon = gtk_image_new_from_stock (MOKO_STOCK_CALL_DIAL, GTK_ICON_SIZE_BUTTON);
    label = gtk_label_new (stock_item.label);
    gtk_box_pack_start (button_vbox, icon, FALSE, FALSE, 0);
    gtk_box_pack_start (button_vbox, label, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (cb_dialer_button_clicked), p_dialer_data);
    gtk_container_add (GTK_CONTAINER (button), alignment);
    gtk_widget_set_name (button, "mokofingerbutton-black");


    gtk_box_pack_start (GTK_BOX (vbox2), button, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 5);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);




    moko_finger_window_set_contents (MOKO_FINGER_WINDOW (window), vbox);

    p_dialer_data->window_dialer = window;

  }

  return 1;
}
