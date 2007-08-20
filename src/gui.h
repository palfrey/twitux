/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 - Alvaro Daniel Morales - <daniel@suruware.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __TWITUX_GUI_H__
#define __TWITUX_GUI_H__

#include <gtk/gtk.h>

#include "main.h"
#include "twitter.h"

GtkWidget *tt_gui_ventana_principal ( TwiTux *twitter );

GtkWidget *tt_gui_create_dialogo_acerca ();

GtkWidget *tt_gui_create_dialogo_login ( GtkWidget **entry_user, GtkWidget **entry_passwd, GtkWidget **check_remember, TwiTux *twitter );

GtkWidget *tt_gui_create_dialogo_what ( TwiTux *twitter, GtkTextBuffer **buffer, TwiTuxTinyzer *tinyzer );

GtkWidget *tt_gui_create_dialogo_direct ( TwiTux *twitter, GtkTextBuffer **buffer, GtkWidget **amigos );

void tt_add_post ( TwiTuxWindow *window, TwiTuxStatus *stauts );

void tt_set_pic ( GtkWidget *tree,  TwiTuxStatus *stauts, const gchar *file );

void tt_add_friend ( TwiTux *twitter, TwiTuxUser *user );

void tt_new_friends_menu ( TwiTux *twitter );

void tt_combo_amigos_add ( gpointer data, gpointer userdata );

GtkStatusIcon *tt_gui_create_notify_icon ( TwiTux *twitter );

void tt_gui_show_bubble ( TwiTux *twitter, const char *mensaje );

GtkWidget *tt_gui_create_dialog_add_fiend ( TwiTux *twitter, GtkWidget **entry_user );

#endif /* __TWITUX_GUI_H__ */
