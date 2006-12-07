/*
 *  libmokoui -- OpenMoko Application Framework UI Library
 *
 *  Authored By Michael 'Mickey' Lauer <mlauer@vanille-media.de>
 *
 *  Copyright (C) 2006 First International Computer Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; version 2.1 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date$) [$Author$]
 */
#include "moko-paned-window.h"
#include "moko-alignment.h"

#include <gtk/gtktoolbar.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkvpaned.h>

#undef DEBUG_THIS_FILE
#ifdef DEBUG_THIS_FILE
#define moko_debug(fmt,...) g_debug(fmt,##__VA_ARGS__)
#else
#define moko_debug(fmt,...)
#endif

G_DEFINE_TYPE (MokoPanedWindow, moko_paned_window, MOKO_TYPE_WINDOW)

#define MOKO_PANED_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MOKO_TYPE_PANED_WINDOW, MokoPanedWindowPriv));

typedef struct _MokoPanedWindowPriv
{
    GtkVPaned* outerframe;
    GtkVBox* upper;
    MokoAlignment* upperenclosing;
    MokoAlignment* lowerenclosing;
    GtkVBox* lower;
    MokoMenuBox* menubox;
    MokoToolBox* toolbox;

} MokoPanedWindowPriv;

/* add your signals here */
enum {
    MOKO_PANED_WINDOW_SIGNAL,
    LAST_SIGNAL
};

static void moko_paned_window_class_init          (MokoPanedWindowClass *klass);
static void moko_paned_window_init                (MokoPanedWindow      *self);

static guint moko_paned_window_signals[LAST_SIGNAL] = { 0 };

static void moko_paned_window_class_init (MokoPanedWindowClass *klass) /* Class Initialization */
{
    g_type_class_add_private(klass, sizeof(MokoPanedWindowPriv));

    moko_paned_window_signals[MOKO_PANED_WINDOW_SIGNAL] = g_signal_new ("moko_paned_window",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (MokoPanedWindowClass, moko_paned_window),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
}

static void moko_paned_window_init (MokoPanedWindow *self) /* Instance Construction */
{
    moko_debug( "moko_paned_window_init" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    priv->outerframe = gtk_vpaned_new();
    gtk_container_add( GTK_CONTAINER(self), GTK_WIDGET(priv->outerframe) );
    priv->upper = gtk_vbox_new( FALSE, 0 );
    gtk_paned_add1( GTK_PANED(priv->outerframe), GTK_WIDGET(priv->upper) );
    priv->lower = gtk_vbox_new( FALSE, 0 );
    gtk_paned_add2( GTK_PANED(priv->outerframe), GTK_WIDGET(priv->lower) );
    priv->menubox = NULL;
    priv->toolbox = NULL;
}

GtkWidget* moko_paned_window_new() /* Construction */
{
    return GTK_WIDGET(g_object_new(moko_paned_window_get_type(), NULL));
}

void moko_paned_window_clear(MokoPanedWindow *self) /* Destruction */
{
    moko_debug( "moko_paned_window_clear" );
    /* destruct your widgets here */
}

/* PUBLIC API */

MokoMenuBox* moko_paned_window_get_menubox(MokoPanedWindow* self)
{
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    return priv->menubox;
}

void moko_paned_window_set_application_menu(MokoPanedWindow* self, GtkMenu* menu)
{
    moko_debug( "moko_paned_window_set_application_menu" );

    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    if (!priv->menubox )
    {
        priv->menubox = moko_menu_box_new();
        gtk_box_pack_start( GTK_BOX(priv->upper), GTK_WIDGET(priv->menubox), FALSE, FALSE, 0 );
    }
    moko_menu_box_set_application_menu( priv->menubox, menu );
}

void moko_paned_window_set_filter_menu(MokoPanedWindow* self, GtkMenu* menu)
{
    moko_debug( "moko_paned_window_set_filter_menu" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    if (!priv->menubox )
    {
        priv->menubox = moko_menu_box_new();
        gtk_box_pack_start( GTK_BOX(priv->upper), GTK_WIDGET(priv->menubox), FALSE, FALSE, 0 );
    }
    moko_menu_box_set_filter_menu( priv->menubox, menu );
}

void moko_paned_window_set_upper_pane(MokoPanedWindow* self, GtkWidget* child)
{
    moko_debug( "moko_paned_window_set_upper_pane" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);

    priv->upperenclosing = moko_alignment_new();
    gtk_widget_set_name( GTK_WIDGET(priv->upperenclosing), "mokopanedwindow-upper-enclosing" );
    //FIXME put into MokoAlignment class
    gtk_alignment_set_padding( GTK_ALIGNMENT(priv->upperenclosing), 7, 7, 11, 11 ); //FIXME get from style (MokoAlignment::padding)
    gtk_box_pack_end( GTK_BOX(priv->upper), GTK_WIDGET(priv->upperenclosing), TRUE, TRUE, 0 );
    gtk_container_add( GTK_CONTAINER(priv->upperenclosing), child );
}

void moko_paned_window_set_lower_pane(MokoPanedWindow* self, GtkWidget* child)
{
    moko_debug( "moko_paned_window_set_lower_pane" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);

#if 0
    gtk_box_pack_start( GTK_BOX(priv->lower), child, TRUE, TRUE, 0 );
#else
    priv->lowerenclosing = moko_alignment_new();
    gtk_widget_set_name( GTK_WIDGET(priv->lowerenclosing), "mokopanedwindow-lower-enclosing" );
    //FIXME put into MokoAlignment class
    gtk_alignment_set_padding( GTK_ALIGNMENT(priv->lowerenclosing), 7, 7, 11, 11 ); //FIXME get from style (MokoAlignment::padding)
    gtk_box_pack_end( GTK_BOX(priv->lower), GTK_WIDGET(priv->lowerenclosing), TRUE, TRUE, 0 );
    gtk_container_add( GTK_CONTAINER(priv->lowerenclosing), child );
#endif
}

void moko_paned_window_add_toolbox(MokoPanedWindow* self, MokoToolBox* toolbox)
{
    moko_debug( "moko_paned_window_add_toolbox" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    gtk_box_pack_end( GTK_BOX(priv->upper), toolbox, FALSE, FALSE, 0 );
    gtk_box_reorder_child( GTK_BOX(priv->upper), toolbox, 1 );
}

void moko_paned_window_set_fullscreen(MokoPanedWindow* self, gboolean b)
{
    moko_debug( "moko_paned_window_set_fullscreen" );
    MokoPanedWindowPriv* priv = MOKO_PANED_WINDOW_GET_PRIVATE(self);
    if ( b )
    {
        gtk_widget_hide( GTK_WIDGET(priv->upper) );
    }
    else
    {
        gtk_widget_show( GTK_WIDGET(priv->upper) );
    }
}
