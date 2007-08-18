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

#ifndef __TWITUX_CALLBACKS_H__
#define __TWITUX_CALLBACKS_H__

#include <gtk/gtk.h>

void tt_on_window_destroy ( GtkWidget *chooser,  gpointer user_data );

void tt_on_acerca_de ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_acerca_link ( GtkAboutDialog *dialog,const gchar *link, gpointer data );

void tt_on_conectar ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_desconectar ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_recargar ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_ver_estado ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_establecer_home ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_ver_home_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_ver_amigos_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_ver_mi_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_ver_respuestas ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_ver_public_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_friend ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_ver_mensajes_directos ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_what ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_detener ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_twitux ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_bugs ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_mensaje ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_cerrar ( GtkMenuItem *menuitem, gpointer user_data );

void tt_on_close_expand ( GtkButton *button, gpointer user_data );

void tt_on_url_expand ( GtkWidget *url_label, gchar *url, gpointer user_data );

void tt_on_status_changed ( GtkTreeView *treeview, gpointer user_data );

void tt_on_ver_expandir_mensajes ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_tinyze_url ( GtkWidget *widget, gpointer user_data );

void on_icon_activate ( GtkStatusIcon *status_icon, gpointer user_data );

void on_icon_menu ( GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data );

gboolean tt_on_window_close_clicked ( GtkWidget *widget, GdkEvent *event, gpointer user_data );

void tt_on_recargar_timelines ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

void tt_on_ver_burbujas ( GtkCheckMenuItem *checkmenuitem, gpointer user_data );

#endif /* __TWITUX_CALLBACKS_H__ */
