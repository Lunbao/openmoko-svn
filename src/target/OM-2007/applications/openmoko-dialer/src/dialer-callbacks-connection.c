/*  openmoko-callbacks-connection.h
 *
 *  Authored By Tony Guan <tonyguan@fic-sh.com.cn>
 *  Thomas Wood <thomas@o-hand.com>
 *  Michael 'Mickey' Lauer <mlauer@vanille-media.de>
 *
 *  Copyright (C) 2006-2007 OpenMoko, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2.1 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 */

#include "dialer-callbacks-connection.h"
#include "dialer-window-incoming.h"


void
network_registration_cb (MokoGsmdConnection *self, int type, int lac, int cell)
{
  /* network registration */
}

void
incoming_call_cb (MokoGsmdConnection *self, int type, MokoDialerData *data)
{
  /* incoming call */
  window_incoming_show (data);
}

void
incoming_pin_request_cb (MokoGsmdConnection *self, int type, MokoDialerData *data)
{
    g_debug( "incoming pin request for type %d", type );
    gtk_widget_show_all( data->window_pin );
}

gboolean initial_timeout_cb (MokoDialerData* data)
{
    g_debug( "initial timeout" );
    /* check whether PIN window is currently visible -- if not, issue register */
    if ( GTK_WIDGET_MAPPED( data->window_pin ) )
    {
        g_debug( "pin window is visible, delaying call to register" );
    }
    else
    {
        g_debug( "pin window not visible, calling register" );
        moko_gsmd_connection_network_register( data->connection );
    }
    return FALSE;
}