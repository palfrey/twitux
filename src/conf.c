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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gconf/gconf-client.h>

#include "main.h"
#include "conf.h"


void tt_gconf_cargar ( TwiTuxConfig *conf )
{
	conf->client = gconf_client_get_default ();
	
	if ( gconf_client_dir_exists ( conf->client, TT_GCONF_DIR, NULL ) ) {

		conf->user_login = gconf_client_get_string ( conf->client, TT_GCONF_DIR "/usuario", NULL );

		conf->user_passwd = gconf_client_get_string ( conf->client, TT_GCONF_DIR "/clave", NULL );

		conf->home_timeline = gconf_client_get_string ( conf->client, TT_GCONF_DIR "/home_timeline", NULL );

		conf->user_remember = gconf_client_get_bool ( conf->client, TT_GCONF_DIR "/recordar", NULL );

		conf->ver_estado = TRUE;

		if ( gconf_client_dir_exists ( conf->client, TT_GCONF_DIR "/ver_estado", NULL ) ){

			conf->ver_estado = gconf_client_get_bool ( conf->client, TT_GCONF_DIR "/ver_estado", NULL );

		}

		conf->expandir_mensajes = TRUE;

		if ( gconf_client_dir_exists ( conf->client, TT_GCONF_DIR "/expandir_mensajes", NULL ) ){

			conf->expandir_mensajes = gconf_client_get_bool ( conf->client, TT_GCONF_DIR "/expandir_mensajes", NULL );

		}

		conf->ver_burbujas = TRUE;

		if ( gconf_client_dir_exists ( conf->client, TT_GCONF_DIR "/ver_burbujas", NULL ) ){

			conf->ver_burbujas = gconf_client_get_bool ( conf->client, TT_GCONF_DIR "/ver_burbujas", NULL );

		}

		conf->recargar_timelines = TRUE;

		if ( gconf_client_dir_exists ( conf->client, TT_GCONF_DIR "/recargar_timelines", NULL ) ){

			conf->recargar_timelines = gconf_client_get_bool ( conf->client, TT_GCONF_DIR "/recargar_timelines", NULL );

		}

	} else {

		g_warning ( "No config" );
		
		// Valores por defecto GUI
		conf->ver_estado = TRUE;

		conf->expandir_mensajes = TRUE;

		conf->ver_burbujas = TRUE;

		conf->recargar_timelines = TRUE;

		// Valores por defecto Usuario
		conf->user_remember = FALSE;

	}

}


void tt_gconf_guardar ( TwiTuxConfig *conf )
{
	if ( gconf_client_key_is_writable ( conf->client, TT_GCONF_DIR, NULL ) ) {

		if ( !conf->user_passwd || !conf->user_remember ) conf->user_passwd = "";

		if ( !conf->user_login || !conf->user_remember ) conf->user_passwd = "";

		// Datos de usuario
		if (conf->user_remember) {

			gconf_client_set_string ( conf->client, TT_GCONF_DIR "/usuario", conf->user_login, NULL );

			gconf_client_set_string ( conf->client, TT_GCONF_DIR "/clave", conf->user_passwd, NULL );

		} else {

			gconf_client_set_string (conf->client, TT_GCONF_DIR "/usuario", "", NULL);

			gconf_client_set_string (conf->client, TT_GCONF_DIR "/clave", "", NULL);

		}

		gconf_client_set_string ( conf->client, TT_GCONF_DIR "/home_timeline", conf->home_timeline, NULL );

		gconf_client_set_bool ( conf->client, TT_GCONF_DIR "/recordar", conf->user_remember, NULL );

		// Cosas GUI
		gconf_client_set_bool ( conf->client, TT_GCONF_DIR "/ver_estado", conf->ver_estado, NULL );

		gconf_client_set_bool ( conf->client, TT_GCONF_DIR "/expandir_mensajes", conf->expandir_mensajes, NULL );
		
		gconf_client_set_bool ( conf->client, TT_GCONF_DIR "/ver_burbujas", conf->ver_burbujas, NULL );
		
		gconf_client_set_bool ( conf->client, TT_GCONF_DIR "/recargar_timelines", conf->recargar_timelines, NULL );
		
	} else {

		g_warning ( "Cannot save the configuration" );

	}

}


void tt_gconf_free ( TwiTuxConfig *conf )
{
	if ( conf->user_login ) g_free ( conf->user_login );
	if ( conf->user_login ) g_free ( conf->user_passwd );
	if ( conf->user_login ) g_free ( conf->home_timeline );

	g_object_unref ( conf->client );

}
