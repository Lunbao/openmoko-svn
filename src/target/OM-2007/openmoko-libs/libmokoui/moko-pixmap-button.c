/*  moko-pixmap-button.c
 *
 *  Authored By Michael 'Mickey' Lauer <mlauer@vanille-media.de>
 *
 *  Copyright (C) 2006 Vanille-Media
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
 *  Current Version: $Rev$ ($Date) [$Author: mickey $]
 */

#include "moko-pixmap-button.h"

#include <gtk/gtkmenu.h>

#undef DEBUG_THIS_FILE
#ifdef DEBUG_THIS_FILE
#define moko_debug(fmt,...) g_debug(fmt,##__VA_ARGS__)
#else
#define moko_debug(fmt,...)
#endif

G_DEFINE_TYPE (MokoPixmapButton, moko_pixmap_button, GTK_TYPE_BUTTON);

#define MOKO_PIXMAP_BUTTON_GET_PRIVATE(o)     (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOKO_TYPE_PIXMAP_BUTTON, MokoPixmapButtonPrivate))

#define CHILD_SPACING 1

typedef struct _MokoPixmapButtonPrivate
{
    GtkMenu *menu;
    GtkWidget *buttonvbox;
    GtkWidget *actionbtnlowerlabel;
    GtkWidget *actionbtnstockimage;
} MokoPixmapButtonPrivate;

static void
moko_pixmap_button_size_request (GtkWidget *widget, GtkRequisition *requisition);

static void
moko_pixmap_button_dispose (GObject *object)
{
    if (G_OBJECT_CLASS (moko_pixmap_button_parent_class)->dispose)
        G_OBJECT_CLASS (moko_pixmap_button_parent_class)->dispose (object);
}

static void
moko_pixmap_button_finalize (GObject *object)
{
    G_OBJECT_CLASS (moko_pixmap_button_parent_class)->finalize (object);
}

static void
moko_pixmap_button_class_init (MokoPixmapButtonClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    /* register private data */
    g_type_class_add_private (klass, sizeof (MokoPixmapButtonPrivate));

    /* hook virtual methods */
    widget_class->size_request = moko_pixmap_button_size_request;

    /* install properties */
    gtk_widget_class_install_style_property( widget_class, g_param_spec_boxed(
        "size-request",
        "Size Request",
        "Sets the widget size request to a fixed value",
        GTK_TYPE_BORDER, G_PARAM_READABLE) );

    object_class->dispose = moko_pixmap_button_dispose;
    object_class->finalize = moko_pixmap_button_finalize;
}

/* callbacks */
static void
cb_menu_position_func (GtkMenu *menu, int *x, int *y, gboolean *push_in, MokoPixmapButton  *button)
{
    GtkAllocation* allocation = &GTK_WIDGET(button)->allocation;
    GtkRequisition req;
    GtkRequisition menu_req;
    GtkOrientation orientation;
    GtkTextDirection direction;

    gdk_window_get_origin(GTK_BUTTON(button)->event_window, x, y);
    moko_debug( "menu popup @ %d, %d", *x, *y );

    *y += allocation->height;

    moko_debug( "size allocate = %d, %d * %d, %d", allocation->x, allocation->y, allocation->width, allocation->height );
    *push_in = TRUE;

    moko_debug( "menu popup @ %d, %d", *x, *y );
}

static void
cb_button_clicked(MokoPixmapButton* self, gpointer data)
{
    MokoPixmapButtonPrivate *priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);

    if (!priv->menu)
        return;

    if (!GTK_WIDGET_VISIBLE(priv->menu))
    {
        /* we get here only when the menu is activated by a key
         * press, so that we can select the first menu item */
        gtk_menu_popup (priv->menu, NULL, NULL,
                        (GtkMenuPositionFunc) cb_menu_position_func,
                        self, 0, gtk_get_current_event_time ());
    }
}

static void
moko_pixmap_button_init (MokoPixmapButton *self)
{
    MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);
    
    moko_debug( "moko_pixmap_button_init" );
    gtk_button_set_focus_on_click( GTK_BUTTON(self), FALSE ); //FIXME probably don't need this when focus is invisible
    GTK_WIDGET_UNSET_FLAGS( GTK_WIDGET(self), GTK_CAN_FOCUS); // The default value of can-focus is TRUE, So it is necessory

    priv->buttonvbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (self), priv->buttonvbox);
    
    g_signal_connect( G_OBJECT(self), "clicked", G_CALLBACK(cb_button_clicked), NULL );
}

static void
moko_pixmap_button_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
    moko_debug( "moko_pixmap_button_size_request" );
    GtkButton *button = GTK_BUTTON (widget);
    GtkBorder default_border;
    GtkBorder* size_request; // modified
    gint focus_width;
    gint focus_pad;

    //gtk_button_get_props (button, &default_border, NULL, NULL); //FIXME what are we going to do w/ default borders?
    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          "size-request", &size_request, // modified
                          NULL);
    
    if ( size_request && size_request->left + size_request->right + size_request->top + size_request->bottom ) // new fixed thing
    {
        moko_debug( "moko_pixmap_button_size_request: style requested size = '%d x %d'", size_request->right, size_request->bottom );
        requisition->width = size_request->right;
        requisition->height = size_request->bottom;

        if (GTK_BIN (button)->child && GTK_WIDGET_VISIBLE (GTK_BIN (button)->child))
        {
            GtkRequisition child_requisition;
            gtk_widget_size_request (GTK_BIN (button)->child, &child_requisition);
        }
        
        //FIXME why should I resize label to set right position on MokoPixmapButton
        MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (widget);
        if ( priv->actionbtnlowerlabel )
        {
            //gtk_widget_set_size_request(priv->actionbtnstockimage, size_request->right, size_request->bottom/2 - 1);
            gtk_widget_set_size_request(priv->actionbtnlowerlabel, size_request->right, size_request->bottom/2 + 1);
        }
        
    }
    else // old dynamic routine
    {
        requisition->width = (GTK_CONTAINER (widget)->border_width + CHILD_SPACING +
                GTK_WIDGET (widget)->style->xthickness) * 2;
        requisition->height = (GTK_CONTAINER (widget)->border_width + CHILD_SPACING +
                GTK_WIDGET (widget)->style->ythickness) * 2;

        if (GTK_WIDGET_CAN_DEFAULT (widget))
        {
            requisition->width += default_border.left + default_border.right;
            requisition->height += default_border.top + default_border.bottom;
        }

        if (GTK_BIN (button)->child && GTK_WIDGET_VISIBLE (GTK_BIN (button)->child))
        {
            GtkRequisition child_requisition;

            gtk_widget_size_request (GTK_BIN (button)->child, &child_requisition);

            requisition->width += child_requisition.width;
            requisition->height += child_requisition.height;
        }

        requisition->width += 2 * (focus_width + focus_pad);
        requisition->height += 2 * (focus_width + focus_pad);
    }
}

/* public API */
GtkWidget*
moko_pixmap_button_new (void)
{
    return GTK_WIDGET(g_object_new(moko_pixmap_button_get_type(), NULL));
}

void
moko_pixmap_button_set_menu (MokoPixmapButton* self, GtkMenu* menu)
{
    MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);
    g_assert( !priv->menu ); //FIXME what's canon for these things? a) Error out or b) just don't do it or c) free the old menu and set the new one?
    priv->menu = menu;
}


void
moko_pixmap_button_set_action_btn_upper_stock (MokoPixmapButton* self, const gchar *stock_name)
{
    MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);
	  
    if ( priv->actionbtnstockimage )
        return;
	  
    GtkWidget *upperalignment = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_box_pack_start (GTK_BOX (priv->buttonvbox), upperalignment, TRUE, TRUE, 0);
    
    priv->actionbtnstockimage = gtk_image_new_from_stock (stock_name, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (upperalignment), priv->actionbtnstockimage);
	  
    gtk_misc_set_alignment (GTK_MISC (priv->actionbtnstockimage), 0.5, 0);
}

void
moko_pixmap_button_set_action_btn_lower_label (MokoPixmapButton* self, const gchar *label)
{
    MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);

    if ( priv->actionbtnlowerlabel )
        return;
    
    GtkWidget *loweralignment = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_box_pack_start (GTK_BOX (priv->buttonvbox), loweralignment, TRUE, TRUE, 0);
    
    priv->actionbtnlowerlabel = gtk_label_new (label);
    
    gtk_container_add (GTK_CONTAINER (loweralignment), priv->actionbtnlowerlabel);
    
    gtk_misc_set_alignment (GTK_MISC (priv->actionbtnlowerlabel), 0.5, 0);
    
}

void
moko_pixmap_button_set_action_btn_center_stock (MokoPixmapButton* self, const gchar *stock_name)
{
    MokoPixmapButtonPrivate* priv = MOKO_PIXMAP_BUTTON_GET_PRIVATE (self);
	  
    if ( priv->actionbtnstockimage )
        return;

    //FIXME why should we use two alignments to center stock image?
    GtkWidget *upperalignment = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_box_pack_start (GTK_BOX (priv->buttonvbox), upperalignment, TRUE, TRUE, 0);

    GtkWidget *loweralignment = gtk_alignment_new (1, 0.5, 0, 0);
    gtk_box_pack_start (GTK_BOX (priv->buttonvbox), loweralignment, TRUE, TRUE, 0);


    priv->actionbtnstockimage = gtk_image_new_from_stock (stock_name, GTK_ICON_SIZE_DND);
    gtk_container_add (GTK_CONTAINER (loweralignment), priv->actionbtnstockimage);
	  
    gtk_misc_set_alignment (GTK_MISC (priv->actionbtnstockimage), 0.5, 0);
}

