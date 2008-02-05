/* opkg_state.c - the opkg package management system

   Thomas Wood <thomas@openedhand.com>

   Copyright (C) 2008 by OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/



typedef enum _opkg_state {
  OPKG_STATE_NONE,
  OPKG_STATE_DOWNLOADING_PKG,
  OPKG_STATE_INSTALLING_PKG,
  OPKG_STATE_CONFIGURING_PKG,
  OPKG_STATE_UPGRADING_PKG,
  OPKG_STATE_REMOVING_PKG,
  OPKG_STATE_DOWNLOADING_REPOSITORY,
  OPKG_STATE_VERIFYING_REPOSITORY_SIGNITURE
} opkg_state_t;

