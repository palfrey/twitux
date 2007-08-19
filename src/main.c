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


#include "config.h"

#include <string.h>
#include <gtk/gtk.h>

#include <libtwitux/twitux-conf.h>

#include "main.h"
#include "gui.h"
#include "callbacks.h"
#include "gcommon.h"
#include "common.h"
#include "network.h"
#include "twitux-preferences.h"

int main ( int argc, char *argv[] )
{
	TwiTux *twitter;
	gchar  *localedir;
	gchar  *home_timeline;

	localedir = twitux_paths_get_locale_path ();
	bindtextdomain (GETTEXT_PACKAGE, localedir);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_free (localedir);

	// Un amigo de #gnome me dijo que hiciera esto ^^
	g_type_init ();

	// Trabajare con multitarea
	g_thread_init ( NULL );
	gdk_threads_init ();
	gdk_threads_enter ();

	// Inicio GTK+
	gtk_set_locale ();
	gtk_init ( &argc, &argv );

	// Inicio dbus
	notify_init ( GETTEXT_PACKAGE );

	// Inicio Twitux! :)
	twitter = g_new0 (TwiTux, 1);
	twitter->principal = g_new0 (TwiTuxWindow, 1);
	twitter->principal->expand = g_new0 (TwiTuxExpand, 1);

	// Setear el last id por defecto
	twitter->last_id = TT_NO_LAST_ID;

	// Inicio libsoup
	tt_network_load ( twitter );

	// Icono de notificacion
	twitter->notify_icon = tt_gui_create_notify_icon ( twitter );

	// Timeout detenido al principio
	twitter->timeout_id = TT_TIMEOUT_STOPPED;

	// Construyo la ventana principal
	twitter->principal->ventana = tt_gui_ventana_principal ( twitter );

	// Carpeta de configuracion y cache
	twitter->config_dir = tt_get_home_dir ();

	if ( !tt_is_dir_exist ( twitter->config_dir ) ){

		tt_crear_dir ( twitter->config_dir );

	}
	
	// Carpeta cache
	twitter->cache_files_dir = g_build_path ( G_DIR_SEPARATOR_S, twitter->config_dir, TT_CACHE_DIR, NULL );

	if ( !tt_is_dir_exist ( twitter->cache_files_dir ) ){

		tt_crear_dir ( twitter->cache_files_dir );

	}

	// Carpeta cache de imagenes
	twitter->cache_images_dir = g_build_path ( G_DIR_SEPARATOR_S, twitter->config_dir, TT_IMAGE_DIR, NULL );

	if ( !tt_is_dir_exist ( twitter->cache_images_dir  ) ){

		tt_crear_dir ( twitter->cache_images_dir  );

	}

	// Lista de amigos
	twitter->friends = NULL;

	// Muestro la ventana principal
	gtk_widget_show ( twitter->principal->ventana );

	// Ocultar el Paned
	gtk_widget_set_size_request ( twitter->principal->expand->frame_expand, -1, 1 );

	// Miscelanega
	twitter->processing = FALSE;

	// Let's get the home timeline
	twitux_conf_get_string (twitux_conf_get(),
								  TWITUX_PREFS_TWEETS_HOME_TIMELINE,
								  &home_timeline);
	if (!home_timeline) {
		home_timeline = g_strdup (TWITTER_PUBLIC_TIMELINE);
	}

	tt_network_get_timeline ( twitter, home_timeline, TRUE );
	g_free (home_timeline);

	// Bucle principal
	gtk_main ();

	// Cerrar todas las conexiones
	soup_session_abort ( twitter->conexion );

	// Termino multitarea
	gdk_threads_leave ();

	// Borro lista de amigos
	tt_clear_friends ( twitter->friends );

	// Cerrar libsoup
	g_object_unref ( twitter->conexion );

	// Cierro dbus
	notify_uninit ();

	/* Let's shutdown gconf */
	twitux_conf_shutdown ();

	// Libero memoria y me voy
	g_free ( twitter->cache_images_dir );
	g_free ( twitter->cache_files_dir );
	g_free ( twitter->config_dir );
	g_free ( twitter->current_timeline );
	
	g_free ( twitter->principal );
	g_free ( twitter );

	return 0; 
}
