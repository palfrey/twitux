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
 
#ifndef __TWITUX_MAIN_H__
#define __TWITUX_MAIN_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <libnotify/notify.h>

#define TT_CACHE_DIR "cache"
#define TT_IMAGE_DIR "images"

#define TT_TIMEOUT_STOPPED 0
#define TT_TIMEOUT_STEP 180000 // 3 minutos

#define TT_NO_LAST_ID 0

#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(String) (const char *) gettext(String)
#else
#  define _(String) (String)
#endif

enum
{
	IMAGEN_COLUMN,
	TEXTO_COLUMN,
	OBJECT_COLUMN,
	N_COLUMNS
};


typedef struct _TwiTuxExpand TwiTuxExpand;

struct _TwiTuxExpand
{
	GtkWidget *frame_expand;

	GtkWidget *label_autor;
	GtkWidget *sexy_label;
	GtkWidget *image;
};

typedef struct _TwiTuxWindow TwiTuxWindow;

struct _TwiTuxWindow
{
	TwiTuxExpand *expand;

	GtkWidget *ventana;
		
	GtkWidget *barra_menu;
	GtkWidget *barra_estado;
	
	guint barra_estado_cid;

	GtkWidget *menuitem_amigos_timeline;
	GtkWidget *menuitem_amigos;
	GtkWidget *menuitem_mi_timeline;
	GtkWidget *menuitem_mensajes_directos;
	GtkWidget *menuitem_respuestas;
	GtkWidget *menuitem_conectar;
	GtkWidget *menuitem_desconectar;
	GtkWidget *menuitem_enviar_estado;
	GtkWidget *menuitem_detener;
	GtkWidget *menuitem_enviar_mensaje;
	GtkWidget *menuitem_recargar;

	GtkWidget *lista_tree;

	GtkWidget *panel_principal;

};


typedef struct _TwiTux TwiTux;

struct _TwiTux
{
	TwiTuxWindow *principal;

	SoupSession *conexion;

	GList *friends;

	gboolean processing;

	gboolean conectado;

	gchar *config_dir;
	gchar *cache_images_dir;
	gchar *cache_files_dir;

	gchar *current_timeline;

	GtkStatusIcon *notify_icon;

	guint timeout_id;

	NotifyNotification *bubble;

	gint last_id;

};


typedef struct _TwiTuxTinyzer TwiTuxTinyzer;

struct _TwiTuxTinyzer
{
	GtkWidget *entry_long;
	GtkWidget *entry_short;
	GtkWidget *button_ok;

	TwiTux *twitter;
};


#define TWITUX_TINYZER(obj) ((TwiTuxTinyzer *)obj)

#define TWITUX_EXPANDER(obj) ((TwiTuxExpand *)obj)

#define TWITUX_TWITUX(obj) ((TwiTux *)obj)

#define TWITUX_TWITUX_WINDOW(obj) ((TwiTuxWindow *)obj)

#endif
