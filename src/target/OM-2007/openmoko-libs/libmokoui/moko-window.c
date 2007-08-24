/*
 *  libmokoui -- OpenMoko Application Framework UI Library
 *
 *  Authored by Michael 'Mickey' Lauer <mlauer@vanille-media.de>
 *  Based on hildon-window.c (C) 2006 Nokia Corporation.
 *
 *  Copyright (C) 2006-2007 OpenMoko Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  Current Version: $Rev$ ($Date$) [$Author$]
 */

#include "moko-application.h"
#include "moko-window.h"

#include <gtk/gtkentry.h>
#include <gtk/gtktextview.h>

#include <gdk/gdkx.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <string.h>

//#define DEBUG_THIS_FILE
#undef DEBUG_THIS_FILE
#ifdef DEBUG_THIS_FILE
#define moko_debug(fmt,...) g_debug(fmt,##__VA_ARGS__)
#define moko_debug_minder(predicate) moko_debug( __FUNCTION__ ); g_return_if_fail(predicate)
#else
#define moko_debug(...)
#endif

/* add your signals here */
enum {
    MOKO_WINDOW_SIGNAL,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_IS_TOPMOST,
};

static void moko_window_class_init(MokoWindowClass *klass);
static void moko_window_init(MokoWindow *self);
static void moko_window_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec);
static void moko_window_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec);
static void moko_window_notify(GObject* gobject, GParamSpec* param);
static void moko_window_is_topmost_notify(MokoWindow* self);

static guint moko_window_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (MokoWindow, moko_window, GTK_TYPE_WINDOW)

#define MOKO_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOKO_TYPE_WINDOW, MokoWindowPrivate));

typedef struct _MokoWindowPrivate
{
    gboolean is_fullscreen;
    gboolean is_topmost;
} MokoWindowPrivate;

static void moko_window_class_init(MokoWindowClass *klass) /* Class Initialization */
{
    moko_debug( "moko_window_class_init" );

    /* register private data */
    g_type_class_add_private( klass, sizeof(MokoWindowPrivate) );

    /* hook virtual methods */
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = moko_window_set_property;
    object_class->get_property = moko_window_get_property;
    object_class->notify = moko_window_notify;

    /* install signals */
    moko_window_signals[MOKO_WINDOW_SIGNAL] = g_signal_new ("moko_window",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (MokoWindowClass, moko_window),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    /* install properties */
    g_object_class_install_property (object_class, PROP_IS_TOPMOST,
                g_param_spec_boolean ("is-topmost",
                "Is top-most",
                "Whether the window is currently activated by the window "
                "manager",
                FALSE,
                G_PARAM_READABLE));
}

static void moko_window_init(MokoWindow *self) /* Instance Construction */
{
    moko_debug( "moko_window_init" );
    gtk_widget_set_size_request( GTK_WIDGET(self), 480, 640 ); //FIXME get from style
    MokoApplication* app = moko_application_get_instance();
    if ( !moko_application_get_main_window( app ) )
        moko_application_set_main_window( app, self );
    moko_application_add_window( app, self );
}

GtkWidget* moko_window_new() /* Construction */
{
    return GTK_WIDGET(g_object_new(moko_window_get_type(), NULL));
}

void moko_window_clear(MokoWindow *self) /* Destruction */
{
    /* destruct your widgets here */
}

static void
moko_window_set_property(GObject* object, guint property_id, const GValue* value, GParamSpec* pspec)
{
    switch (property_id) {

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
moko_window_get_property(GObject* object, guint property_id, GValue* value, GParamSpec* pspec)
{
    MokoWindowPrivate* priv = MOKO_WINDOW_GET_PRIVATE(object);

    switch (property_id) {

    case PROP_IS_TOPMOST:
            g_value_set_boolean( value, priv->is_topmost );
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
        break;
    }
}

static void
moko_window_notify(GObject* gobject, GParamSpec* param)
{
    moko_debug( "moko_window_notify '%s'", param->name );
    MokoWindow* window = MOKO_WINDOW(gobject);

    if (strcmp(param->name, "title") == 0)
    {
        moko_debug( "update window title" );
        //moko_window_update_title(window);
    }
    else if (strcmp(param->name, "is-topmost") == 0)
    {
        moko_window_is_topmost_notify(window);
    }

    if (G_OBJECT_CLASS(moko_window_parent_class)->notify)
        G_OBJECT_CLASS(moko_window_parent_class)->notify( gobject, param );
}

static void
moko_window_is_topmost_notify(MokoWindow* self)
{
    moko_debug( "moko_window_is_topmost_notify" );
    MokoWindowPrivate* priv = MOKO_WINDOW_GET_PRIVATE(self);
    if (priv->is_topmost)
    {
        moko_debug( "-- I am topmost now :D" );
    }
    else
    {
        moko_debug( "-- I am no longer topmost :(" );
    }
}

/*
 * Compare the window that was last topped, and act consequently
 */
void
moko_window_update_topmost(MokoWindow* self, Window window_id)
{
    moko_debug( "moko_window_update_topmost" );
    MokoWindowPrivate* priv = MOKO_WINDOW_GET_PRIVATE(self);
    Window my_window;

    /* don't test the window unless it is realised */
    if (!GTK_WIDGET (self)->window)
       return;

    my_window = GDK_WINDOW_XID (GTK_WIDGET (self)->window);

    if (window_id == my_window)
    {
        if (!priv->is_topmost)
        {
            priv->is_topmost = TRUE;
            g_object_notify( G_OBJECT(self), "is-topmost" );
        }
    }
    else if (priv->is_topmost)
    {
        /* Should this go in the signal handler? */
        GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(self));

        if (GTK_IS_ENTRY(focus))
            gtk_im_context_focus_out(GTK_ENTRY(focus)->im_context);
        if (GTK_IS_TEXT_VIEW (focus))
            gtk_im_context_focus_out(GTK_TEXT_VIEW(focus)->im_context);

        priv->is_topmost = FALSE;
        g_object_notify( G_OBJECT(self), "is-topmost" );
    }
}

void
moko_window_set_status_message (MokoWindow *self, gchar *message)
{
  GtkWidget *window = GTK_WIDGET (self);

  g_return_if_fail (MOKO_IS_WINDOW (self));

  XChangeProperty(GDK_WINDOW_XDISPLAY (window->window), GDK_WINDOW_XID(window->window),
    gdk_x11_get_xatom_by_name ("_MOKO_STATUS_MESSAGE"),
    XA_STRING,
    8,
    PropModeReplace,
    (guchar*)message,
    strlen (message) + 1);
}

void
moko_window_set_status_progress (MokoWindow *self, gdouble progress)
{
  GtkWidget *window = GTK_WIDGET (self);
  g_return_if_fail (MOKO_IS_WINDOW (self));

  XChangeProperty(GDK_WINDOW_XDISPLAY (window->window), GDK_WINDOW_XID(window->window),
    gdk_x11_get_xatom_by_name ("_MOKO_STATUS_PROGRESS"),
    XA_STRING,
    8,
    PropModeReplace,
    (unsigned char *)&progress,
    sizeof (progress));
}