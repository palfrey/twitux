#if !defined TWITUX_GCONF_H
#define TWITUX_GCONF_H
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
 
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#define TT_GCONF_DIR "/apps/twitux"


typedef struct _TwiTuxConfig TwiTuxConfig;

struct _TwiTuxConfig
{
	GConfClient *client;

	gchar *user_login;
	gchar *user_passwd;

	gboolean user_remember;

	gboolean ver_estado;
	
	gboolean expandir_mensajes;

	gchar *home_timeline;
	
	gboolean ver_burbujas;

	gboolean recargar_timelines;

};

#define TWITUX_CONFIG(obj) ((TwiTuxConfig *)obj)

void tt_gconf_cargar (TwiTuxConfig *conf);

void tt_gconf_free (TwiTuxConfig *conf);

void tt_gconf_guardar (TwiTuxConfig *conf);

#endif
