/*
 *  sms-membership-window.c
 *  
 *  Authored By Alex Tang <alex@fic-sh.com.cn>
 *
 *  Copyright (C) 2006-2007 OpenMoko Inc
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2.1 of the license.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Public License for more details.
 *  
 *  Current Version: $Rev$ ($Date: 2006/10/05 17:38:14 $) [$Author: alex $]
 */

#include "sms-membership-window.h"
#include <libmokoui/moko-pixmap-button.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkviewport.h>

#include <pango/pango-font.h>

#include <glib/gmain.h>

G_DEFINE_TYPE (SmsMembershipWindow, sms_membership_window, MOKO_TYPE_WINDOW)

#define SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(o)   (G_TYPE_INSTANCE_GET_PRIVATE ((o), SMS_TYPE_MEMBERSHIP_WINDOW, SmsMembershipWindowPrivate))

typedef struct _SmsMembershipWindowPrivate SmsMembershipWindowPrivate;

struct _SmsMembershipWindowPrivate
  {
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* folderbox;
    GtkWidget* eventbox;
    GtkWidget* menubox;
    GtkWidget* titleLabel;
    GtkWidget* fromLabel;
    GtkWidget* subjectLabel;
    GtkWidget* closebutton;
    GtkWidget* radioBtnBox;
    GtkWidget* radioAlign;
    GtkWidget* msgViewAlign;
    GtkWidget* msgViewWin;
    GtkTreeModel* filter;
    GtkWidget* view;
    GtkListStore* liststore;
    GSList* rdoBtnList;
  };

typedef struct _SmsMembershipRunInfo
  {
    SmsMembershipWindow *dialog;
    gint response_id;
    GMainLoop *loop;
    gboolean destroyed;
  }
SmsMembershipRunInfo;

static void sms_membership_window_close(SmsMembershipWindow* self);

/*static void
shutdown_loop (SmsMembershipRunInfo *ri)
{
    if (g_main_loop_is_running (ri->loop))
        g_main_loop_quit (ri->loop);
}*/

static void
sms_membership_window_dispose(GObject* object)
{
  if (G_OBJECT_CLASS (sms_membership_window_parent_class)->dispose)
    G_OBJECT_CLASS (sms_membership_window_parent_class)->dispose (object);
}

static void
sms_membership_window_finalize(GObject* object)
{
  G_OBJECT_CLASS (sms_membership_window_parent_class)->finalize (object);
}

static void
sms_membership_window_class_init(SmsMembershipWindowClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private (klass, sizeof(SmsMembershipWindowPrivate));

  object_class->dispose = sms_membership_window_dispose;
  object_class->finalize = sms_membership_window_finalize;
}

GtkWidget*
sms_membership_window_new(void)
{
  return GTK_WIDGET(g_object_new(SMS_TYPE_MEMBERSHIP_WINDOW, NULL));
}

static void sms_membership_window_close(SmsMembershipWindow* self)
{
  GtkWidget *widget = GTK_WIDGET(self);
  GdkEvent *event;

  event = gdk_event_new( GDK_DELETE );

  event->any.window = g_object_ref(widget->window);
  event->any.send_event = TRUE;

  gtk_main_do_event( event );
  gdk_event_free( event );
}

gboolean membership_filter_changed(GtkWidget* widget, gchar* text, SmsMembershipWindow* self)
{
  g_debug("changed to %s folder",text);
  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);
  self->currentfolder = g_strdup(text);
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER(priv->filter));

  return FALSE;
}

void sms_membership_window_show (SmsMembershipWindow* self)
{
  GtkWidget *widget = GTK_WIDGET(self);
  gtk_widget_show (widget);
}

static void
sms_membership_window_init(SmsMembershipWindow* self)
{
  MokoWindow* parent = (MokoWindow*)moko_application_get_main_window( moko_application_get_instance() );
  if ( parent )
    {
      gtk_window_set_transient_for( GTK_WINDOW(self), GTK_WINDOW(parent) );
      gtk_window_set_modal( GTK_WINDOW(self), TRUE );
      gtk_window_set_destroy_with_parent( GTK_WINDOW(self), TRUE );
    }

  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);
  self->currentfolder = g_strdup("Inbox");
  //Set title
  priv->liststore = NULL;
  priv->vbox = gtk_vbox_new( FALSE, 0 );

  priv->menubox = moko_menu_box_new();
  gtk_box_pack_start( GTK_BOX(priv->vbox), GTK_WIDGET(priv->menubox), FALSE, FALSE, 0 );

  priv->titleLabel = gtk_label_new( "Message Membership" );
  gtk_window_set_title( GTK_WINDOW(self), "Message Membership" );
  gtk_widget_set_name( GTK_WIDGET(priv->titleLabel), "mokodialogwindow-title-label" );
  priv->eventbox = gtk_event_box_new();
  gtk_container_add( GTK_CONTAINER(priv->eventbox), GTK_WIDGET(priv->titleLabel) );
  gtk_widget_set_name( GTK_WIDGET(priv->eventbox), "mokodialogwindow-title-labelbox" );

  //FIXME get from theme
  gtk_misc_set_padding( GTK_MISC(priv->titleLabel), 0, 6 );
  gtk_widget_show( GTK_WIDGET(priv->titleLabel) );
  gtk_widget_show( GTK_WIDGET(priv->eventbox) );
  gtk_box_pack_start( GTK_BOX(priv->vbox), GTK_WIDGET(priv->eventbox), FALSE, FALSE, 0 );
  priv->folderbox = gtk_vbox_new( FALSE, 0 );

  //Set folder list
  GtkWidget* closebox = gtk_hbox_new( FALSE, 0 );
  PangoFontDescription* font_desc;
  font_desc = pango_font_description_from_string ("bold 12");
  priv->fromLabel = gtk_label_new( "AlexTang" );
  gtk_widget_modify_font (priv->fromLabel, font_desc);
  gtk_widget_set_size_request (priv->fromLabel, 250, -1);
  gtk_misc_set_alignment (GTK_MISC(priv->fromLabel), 0, 0.5);
  priv->subjectLabel = gtk_label_new( "Subject" );
  gtk_widget_set_size_request (priv->subjectLabel, 250, -1);
  gtk_misc_set_alignment (GTK_MISC(priv->subjectLabel), 0, 0.5);

  //set header box: two labels and a closebutton
  GtkWidget* headerbox = gtk_vbox_new( FALSE, 0 );
  gtk_box_set_spacing (GTK_BOX(headerbox),5);
  priv->closebutton = gtk_button_new_with_label ("Close");
  gtk_widget_set_size_request (priv->closebutton, -1, 38);
  GtkAlignment* alignment = GTK_ALIGNMENT(gtk_alignment_new (0.5, 0.5, 1, 1));
  gtk_alignment_set_padding (alignment, 5, 5, 5, 5);
  gtk_container_add (GTK_CONTAINER(alignment), priv->closebutton);
  GtkWidget* hsep = gtk_hseparator_new();
  gtk_widget_set_size_request (hsep, -1, 3);

  //set folder box alignment fbAlign
  GtkAlignment* fbAlign = GTK_ALIGNMENT(gtk_alignment_new (0.5, 0.5, 1, 1));
  gtk_alignment_set_padding (fbAlign, 5, 5, 30, 30);
  gtk_container_add( GTK_CONTAINER(fbAlign), GTK_WIDGET(priv->folderbox) );

  gtk_box_pack_start( GTK_BOX(headerbox), GTK_WIDGET(priv->fromLabel), TRUE, TRUE, 0 );
  gtk_box_pack_start( GTK_BOX(headerbox), GTK_WIDGET(priv->subjectLabel), TRUE, TRUE, 0 );
  gtk_box_pack_start( GTK_BOX(closebox), GTK_WIDGET(headerbox), TRUE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(closebox), GTK_WIDGET(alignment), FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(priv->folderbox), GTK_WIDGET(closebox), FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(priv->folderbox), GTK_WIDGET(hsep), FALSE, FALSE, 0 );
  gtk_box_pack_start( GTK_BOX(priv->vbox), GTK_WIDGET(fbAlign), FALSE, FALSE, 0 );

  gtk_widget_show_all( GTK_WIDGET(priv->vbox) );
  gtk_container_add( GTK_CONTAINER(self), GTK_WIDGET(priv->vbox) );
  g_signal_connect_swapped ( G_OBJECT(priv->closebutton), "clicked", G_CALLBACK(sms_membership_window_close), self );
  g_signal_connect( G_OBJECT(priv->menubox), "filter_changed", G_CALLBACK(membership_filter_changed), self );

}

void membership_rdo_btn_clicked ( GtkButton* button, SmsMembershipWindow* self)
{
  GtkTreeModel* model;
  GtkTreeIter iter;
  GtkTreeIter childiter;
  GtkTreeSelection* selection;
  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->view));
  gboolean has_selection = gtk_tree_selection_get_selected (selection, &model,&iter);
  if (has_selection)
    {
      gchar* folder;
      gtk_tree_model_get (model, &iter, COLUMN_FOLDER, &folder, -1);
      g_debug ("folder is %s",folder);
      self->currentfolder = g_strdup(folder);
      gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(priv->filter),&childiter,&iter);
      gtk_list_store_set(priv->liststore, &childiter, COLUMN_FOLDER, gtk_button_get_label(button), -1);
      gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER(priv->filter));
      g_free(folder);
    }
}

void sms_membership_window_set_menubox(SmsMembershipWindow* self, GSList* folderlist)
{
  GtkWidget* appmenu;
  GtkWidget* filtmenu;

  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);

  /* application menu */
  appmenu = NULL;
  
  GtkWidget* rdobtnbox = gtk_vbox_new(FALSE, 0) ;
  GtkWidget *rdo_btn = NULL;
  GSList *rdo_btn_group;
  GSList* c = folderlist;

  filtmenu = gtk_menu_new();
  for (; c; c = g_slist_next(c) )
    {
      //add folder to filter menu
      gchar* folder = (gchar*) c->data;
      g_debug( "adding folder '%s'", folder );
      gtk_menu_shell_append( GTK_MENU_SHELL( filtmenu ), gtk_menu_item_new_with_label( folder ) );

      //add folder to folder list
      if(!g_strcasecmp(folder,"Inbox"))
        {
          rdo_btn = gtk_radio_button_new_with_label (NULL, folder);
          rdo_btn_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (rdo_btn));
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rdo_btn), TRUE);
        }
      else
        rdo_btn = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (rdo_btn), folder);
      priv->rdoBtnList = g_slist_append (priv->rdoBtnList,rdo_btn);
      g_signal_connect (G_OBJECT(rdo_btn), "released", G_CALLBACK (membership_rdo_btn_clicked), self);
      gtk_box_pack_start (GTK_BOX (rdobtnbox), rdo_btn, FALSE, TRUE, 0);
    }

  //set radio button box alignment
  if (!GTK_IS_ALIGNMENT(priv->radioAlign))
    {
      g_debug("Should be the first fime");
      priv->radioAlign = gtk_alignment_new (0.5, 0.5, 1, 1);
      gtk_alignment_set_padding (GTK_ALIGNMENT(priv->radioAlign), 5, 5, 30, 5);
      priv->radioBtnBox = rdobtnbox;
      gtk_container_add( GTK_CONTAINER(priv->radioAlign), GTK_WIDGET(priv->radioBtnBox) );
      gtk_box_pack_start (GTK_BOX (priv->folderbox), GTK_WIDGET(priv->radioAlign), FALSE, FALSE, 0);
      moko_menu_box_set_application_menu( MOKO_MENU_BOX(priv->menubox), GTK_MENU(appmenu) );
    }
  else
    {
      gtk_container_remove( GTK_CONTAINER(priv->radioAlign), GTK_WIDGET(priv->radioBtnBox) );
      priv->radioBtnBox = rdobtnbox;
      gtk_container_add( GTK_CONTAINER(priv->radioAlign), GTK_WIDGET(priv->radioBtnBox) );
    }
  moko_menu_box_set_filter_menu( MOKO_MENU_BOX(priv->menubox), GTK_MENU(filtmenu) );
  gtk_widget_show (priv->menubox);
  gtk_widget_show_all (priv->vbox);
}

void
membership_cell_data_func (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer          user_data)
{
  gint status;

  gtk_tree_model_get(model, iter, COLUMN_STATUS, &status, -1);
  if (status == UNREAD)
    g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD, "weight-set", TRUE, NULL);
  else
    g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD, "weight-set", FALSE, NULL);
}

void membership_cursor_changed(GtkTreeSelection    *selection,
                               SmsMembershipWindow *self)
{
  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);
  GtkTreeModel* model;
  GtkTreeIter iter;
  message* msg;

  if ( gtk_tree_selection_get_selected( selection, &model, &iter ) )
    {
      msg = g_malloc(sizeof(message));
      gtk_tree_model_get( model, &iter, COLUMN_FROM, &msg->name, -1 );
      gtk_tree_model_get( model, &iter, COLUMN_SUBJECT, &msg->subject, -1 );
      gtk_tree_model_get( model, &iter, COLUMN_FOLDER, &msg->folder, -1 );
      gtk_tree_model_get( model, &iter, COLUMN_STATUS, &msg->status, -1);

      gtk_label_set_text (GTK_LABEL(priv->fromLabel), msg->name);
      gtk_label_set_text (GTK_LABEL(priv->subjectLabel), msg->subject);
      g_debug ("radio list length: %d",g_slist_length(priv->rdoBtnList));

      GSList* c;
      GtkWidget *elem;
      for( c =priv->rdoBtnList; c; c=g_slist_next(c))
        {
          elem = c->data;
          if (!g_strcasecmp (msg->folder,gtk_button_get_label(GTK_BUTTON(elem))))
            {
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(elem), TRUE);
              break;
            }
        }
    }
}

gboolean membership_filter_visible_function (GtkTreeModel* model, GtkTreeIter* iter, SmsMembershipWindow* self)
{
  gchar* folder;
  gtk_tree_model_get (model, iter, COLUMN_FOLDER, &folder, -1);

  if(!g_strcasecmp(folder,self->currentfolder)){
    g_free (folder);
    return TRUE;
  }
  else{
    g_free (folder);
    return FALSE;
  }
}

void sms_membership_window_set_messages (SmsMembershipWindow* self,
    GtkListStore* liststore)
{
  GtkCellRenderer* ren;
  GtkTreeViewColumn* column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column, "From");


  SmsMembershipWindowPrivate* priv = SMS_MEMBERSHIP_WINDOW_GET_PRIVATE(self);
  priv->liststore = liststore;
  priv->filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (priv->liststore),NULL);
  priv->view = moko_tree_view_new_with_model( GTK_TREE_MODEL (priv->filter));
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->filter),
                                          (GtkTreeModelFilterVisibleFunc)membership_filter_visible_function,
                                          self,
                                          NULL);
  /* Add status picture */
  ren = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, ren, FALSE);
  gtk_tree_view_column_set_attributes(column, ren, "pixbuf", COLUMN_ICON, NULL);

  /* add message from name */
  ren = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, ren, TRUE);
  gtk_tree_view_column_set_attributes(column, ren, "text", COLUMN_FROM, NULL);

  /* Bold if UNREAD */
  gtk_tree_view_column_set_cell_data_func (column, ren, membership_cell_data_func, priv->liststore, NULL);
  moko_tree_view_append_column( MOKO_TREE_VIEW(priv->view), column );

  ren = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(column, "Subject");
  gtk_tree_view_column_pack_start(column, ren, TRUE);
  gtk_tree_view_column_set_attributes(column, ren, "text", COLUMN_SUBJECT, NULL);
  gtk_tree_view_column_set_cell_data_func (column, ren, membership_cell_data_func, priv->liststore, NULL);
  moko_tree_view_append_column( MOKO_TREE_VIEW(priv->view), column );

  if (!GTK_IS_ALIGNMENT(priv->msgViewAlign))
    {
      GtkWidget* treeViewAlign = gtk_alignment_new (0.5, 0.5, 1, 1);
      gtk_alignment_set_padding (GTK_ALIGNMENT(treeViewAlign),10,10,10,10);
      priv->msgViewAlign = treeViewAlign;
      priv->msgViewWin = moko_tree_view_put_into_scrolled_window(MOKO_TREE_VIEW(priv->view));
      gtk_container_add (GTK_CONTAINER(priv->msgViewAlign),priv->msgViewWin);
      gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET(priv->msgViewAlign), TRUE, TRUE, 0);
    }
  else
    {
      gtk_container_remove (GTK_CONTAINER(priv->msgViewAlign),priv->msgViewWin);
      priv->msgViewWin = moko_tree_view_put_into_scrolled_window(MOKO_TREE_VIEW(priv->view));
      gtk_container_add (GTK_CONTAINER(priv->msgViewAlign),priv->msgViewWin);
    }
  gtk_widget_show_all (priv->vbox);

  GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->view));
  g_signal_connect( G_OBJECT(selection), "changed", G_CALLBACK(membership_cursor_changed), self);
}
