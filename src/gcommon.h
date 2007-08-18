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

#ifndef __TWITUX_G_COMMON_H__
#define __TWITUX_G_COMMON_H__

#include <gtk/gtk.h>

#include "main.h"


void tt_ver_ocultar_widget ( GtkWidget *widget, gboolean ver );

GdkPixbuf *cargar_pixbuf ( const char *path );

GtkWidget *crear_menu ( const char *texto, GtkWidget *parent );

GtkWidget *crear_stock_mi ( const char *stock, GtkWidget *parent, GtkAccelGroup *accel_group );

GtkWidget *crear_menu_separador ( GtkWidget *parent );

GtkWidget *crear_menu_check ( GtkWidget *parent, const char *txt, gboolean active );

GtkWidget *crear_menu_radio ( GSList *group, const char *txt, gboolean active,  GtkWidget *parent );

void tt_cambiar_mensaje_estado ( TwiTux *twitter, const char *mensaje );

void tt_clear_list ( GtkWidget *lista_tree );

void tt_enable_disable_widgets ( gboolean flag, TwiTuxWindow *window );

void tt_clear_friends ( GList *friends );

void tt_set_networking ( TwiTux *twitter, gboolean flag );

gchar *get_ruta_imagen ( const gchar *file );

#endif /* __TWITUX_G_COMMON_H__ */
