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

#include <string.h>
#include <libsoup/soup.h>
#include <libsexy/sexy.h>

#include <libtwitux/twitux-conf.h>

#include "main.h"
#include "twitter.h"
#include "common.h"
#include "gcommon.h"
#include "network.h"
#include "gui.h"
#include "twitux-preferences.h"

static void tt_on_get_timeline ( SoupMessage *msg, gpointer user_data );
static void tt_on_get_imagen ( SoupMessage *msg, gpointer user_data );
static void tt_on_get_friends ( SoupMessage *msg, gpointer user_data );
static void tt_on_get_mensajes_directos ( SoupMessage *msg, gpointer user_data );
static void tt_on_post_status ( SoupMessage *msg, gpointer user_data );
static void tt_on_send_message ( SoupMessage *msg, gpointer user_data );
static void tt_on_auth ( SoupSession *session, SoupMessage *msg, const char *auth_type, const char *auth_realm, char **username, char **password, gpointer data );

void tt_on_get_tinyurl ( SoupMessage *msg, gpointer user_data );

gboolean tt_network_post_data ( TwiTux *twitter, const gchar *url, gchar *formdata, SoupMessageCallbackFn callback );
gboolean tt_network_get_data ( TwiTux *twitter, const gchar *url, SoupMessageCallbackFn callback, gboolean flag_current );

gchar *tt_network_save_data ( TwiTux *twitter, SoupMessage *msg, const char *file );

gboolean tt_network_check_status ( TwiTux *twitter, SoupMessage *msg );

gboolean tt_update_timeout ( gpointer data );


// Inicio libsoup
void tt_network_load ( TwiTux *twitter  )
{

	twitter->conexion  = soup_session_async_new_with_options ( SOUP_SESSION_MAX_CONNS, 8, NULL );

}


// Para obtener mensajes directos
void tt_network_direct_messages ( TwiTux *twitter )
{

	if ( tt_network_get_data ( twitter, TWITTER_DIRECT_MESSAGES, tt_on_get_mensajes_directos, TRUE ) ) {

		tt_timeout_remove ( twitter );

		tt_cambiar_mensaje_estado ( twitter, _("Loading: Direct messages...") );

		// Setear el last id por defecto
		twitter->last_id = TT_NO_LAST_ID;

	}

}


// Para descargar un timeline
void tt_network_get_timeline ( TwiTux *twitter, const char *url, gboolean restet_lastid )
{
	if ( tt_network_get_data ( twitter, url, tt_on_get_timeline, TRUE ) ) {

		tt_timeout_remove ( twitter );

		tt_cambiar_mensaje_estado ( twitter, _("Loading: Timeline...") );

		if ( restet_lastid ) {

			// Setear el last id por defecto
			twitter->last_id = TT_NO_LAST_ID;

		}

	}

}


// Tinyzar una URL
void tt_network_tinyze ( TwiTuxTinyzer *tinyzer, const char *url )
{
	SoupMessage *msg;

	gchar *tinyze;

	// Creo un POST
	tinyze = g_strdup_printf ( "%s/?url=%s", TINYURL_API_URL, url );

	msg = soup_message_new ( "GET", tinyze );

	soup_session_queue_message ( tinyzer->twitter->conexion, msg, tt_on_get_tinyurl, tinyzer );
	
	g_free ( tinyze );

}


// Para descargar imagenes
void tt_network_get_image ( TwiTux *twitter, TwiTuxStatus *status )
{
	SoupMessage *msg;

	msg = soup_message_new ( "GET", status->user->profile_image_url );

	soup_session_queue_message ( twitter->conexion, msg, tt_on_get_imagen, status );

}


// Para conectarse (obtener lista de amigos)
void tt_network_login ( TwiTux *twitter )
{

	g_signal_connect ( twitter->conexion, "authenticate", G_CALLBACK ( tt_on_auth ), twitter );

	g_signal_connect ( twitter->conexion, "reauthenticate", G_CALLBACK ( tt_on_auth ), twitter );


	if ( tt_network_get_data ( twitter, TWITTER_FRIENDS_LIST, tt_on_get_friends, FALSE ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Conecting...") );

	}

}


// Publicar nuevo estado
void tt_network_post_status ( TwiTux *twitter, gchar *text )
{
	gchar *formdata;
	gchar *urlencoded;

	// URL encodeo
	urlencoded = url_encode_alloc ( text );

	// Creo un POST
	formdata = g_strdup_printf ( "status=%s", urlencoded );

	g_free ( urlencoded );

	// .. y lo envio
	if ( tt_network_post_data ( twitter, TWITTER_POST_STATUS, formdata, tt_on_post_status ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Sending status...") );

	}

}


// Enviar un mensaje directo a un usuario
void tt_network_send_message ( TwiTux *twitter, gchar *friend, gchar *text )
{
	gchar *formdata;
	gchar *urlencoded;

	// URL encodeo
	urlencoded = url_encode_alloc ( text );

	// Creo el POST
	formdata = g_strdup_printf ( "user=%s&text=%s", friend, urlencoded );

	g_free ( urlencoded );

	// .. y lo envio
	if ( tt_network_post_data ( twitter, TWITTER_SEND_MESSAGE, formdata, tt_on_send_message ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Sending message...") );

	}

}


// Callback al recibir los mensajes directos
static void tt_on_get_mensajes_directos ( SoupMessage *msg, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gchar *tmp_file = NULL;

	tt_set_networking ( twitter, FALSE );

	// Checkeo la respuesta HTTP
	if ( !tt_network_check_status ( twitter, msg ) ) return;

	tt_cambiar_mensaje_estado ( twitter, _("Ok, displaying direct messages.") );

	// Guardo datos llegados en archivo temporal
	tmp_file = tt_network_save_data ( twitter, msg, "messages.xml" );

	// Parseo y muestro los mensajes
	if ( !tt_twitter_load_messages ( tmp_file, twitter ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Messages parser error.") );

	}

	g_free ( tmp_file );

}


// Callback al recibir mi lista de amigos
static void tt_on_get_friends ( SoupMessage *msg, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gchar *tmp_file = NULL;

	tt_set_networking ( twitter, FALSE );

	// Checkeo la respuesta HTTP
	if ( !tt_network_check_status ( twitter, msg ) ){

		if ( msg->status_code != 1 ) {

			// Deshabilito controles para usuario loggeado
			tt_enable_disable_widgets ( FALSE, twitter->principal );

			// Flag de iniciada sesion
			twitter->conectado = FALSE;

		}

		return;

	}

	tt_cambiar_mensaje_estado ( twitter, _("Ok, connected :)") );

	// Guardo datos llegados en archivo temporal
	tmp_file = tt_network_save_data ( twitter, msg, "friends.xml" );

	// Habilitar controles de amigos
	tt_enable_disable_widgets ( TRUE, twitter->principal );

	// Flag de iniciada sesion
	twitter->conectado = TRUE;

	// Parseo y cargo el menu amigos
	if ( !tt_twitter_load_friends ( tmp_file, twitter ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Friends parser error.") );

	}

	g_free ( tmp_file );

}


// Callback al recibir timelines
static void tt_on_get_timeline ( SoupMessage *msg, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gchar *tmp_file = NULL;

	tt_set_networking ( twitter, FALSE );

	// Checkeo la respuesta HTTP
	if ( !tt_network_check_status ( twitter, msg ) ) return;

	tt_cambiar_mensaje_estado ( twitter, _("Ok, parsing...") );

	// Auto-update
	tt_timeout_start ( twitter );

	// Guardo datos llegados en archivo temporal
	tmp_file = tt_network_save_data ( twitter, msg, "timeline.xml" );

	// Parseo y muestro el timeline ;)
	if ( tt_twitter_load_timeline ( tmp_file, twitter ) ) {

		tt_cambiar_mensaje_estado ( twitter, _("Ok, timeline loaded.") );

	} else {

		tt_cambiar_mensaje_estado ( twitter, _("Timeline parser error.") );

	}
	
	g_free ( tmp_file );

}


// Respuesta al Tynizar una URL
void tt_on_get_tinyurl ( SoupMessage *msg, gpointer user_data )
{
	TwiTuxTinyzer *tinyzer = TWITUX_TINYZER ( user_data );

	gchar *result;
	gchar *tinyurl;

	gtk_widget_set_sensitive ( tinyzer->entry_long, TRUE );

	gtk_widget_set_sensitive ( tinyzer->button_ok, TRUE );

	// Detenido
	if ( msg->status_code == 1 ) return;

	// Checkeo de respuesta HTTP
	if ( msg->status_code != 200 ){

		sexy_url_label_set_markup ( SEXY_URL_LABEL ( tinyzer->entry_short ), _("Fail, wrong response.") );

		return;

	}

	tinyurl = g_strndup ( msg->response.body, msg->response.length );

	// Muestro la respuesta 
	result = g_strdup_printf ( "Done, copy: <a href='%s'>%s</a>", tinyurl, tinyurl );

	sexy_url_label_set_markup ( SEXY_URL_LABEL ( tinyzer->entry_short ), result );

	g_free ( tinyurl );

	g_free ( result );

}


// Callback al recibir una imágen
static void tt_on_get_imagen ( SoupMessage *msg, gpointer user_data )
{
	TwiTuxStatus *status = TWITUX_STATUS ( user_data );

	// Checkeo la respuesta HTTP
	if ( msg->status_code != 200 ) return;

	// Guardo
	g_file_set_contents (  status->user->profile_image_md5, msg->response.body, msg->response.length , NULL );

	// Cambio la pic en el timeline
	tt_set_pic ( status->tree,  status, status->user->profile_image_md5 );

}


// Callback de respuesta al enviar mensaje
static void tt_on_post_status ( SoupMessage *msg, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	tt_set_networking ( twitter, FALSE );

	// Checkeo la respuesta HTTP
	if ( tt_network_check_status ( twitter, msg ) ){

		tt_cambiar_mensaje_estado ( twitter, _("Ok, status sent! :)") );

	}

	g_free ( msg->request.body );	

}


// Callback al enviar mensaje
static void tt_on_send_message ( SoupMessage *msg, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	tt_set_networking ( twitter, FALSE );

	// Checkeo la respuesta HTTP
	if ( tt_network_check_status ( twitter, msg ) ){

		tt_cambiar_mensaje_estado ( twitter, _("Ok, message sent! :)") );

	}

	g_free ( msg->request.body );	

}


// Pidiendo datos para conectarse
static void tt_on_auth (SoupSession  *session,
						SoupMessage  *msg,
						const char   *auth_type,
						const char   *auth_realm,
						char        **username,
						char        **password,
						gpointer      user_data)
{
	TwituxConf *conf;
	gchar      *user_id;
	gchar      *user_passwd;

	conf = twitux_conf_get ();
	
	/*
	 * TODO: Add check to verify that user id & password has
	 *       been set.
	 */
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_USER_ID,
							&user_id);
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_PASSWORD,
							&user_passwd);

	*username = g_strdup (user_id);
	*password = g_strdup (user_passwd);

	g_free (user_id);
	g_free (user_passwd);
}


// Detiene el timeout que actualiza
gboolean tt_timeout_remove ( TwiTux *twitter )
{

	if ( twitter->timeout_id == TT_TIMEOUT_STOPPED ) return TRUE;

	if ( g_source_remove ( twitter->timeout_id ) ) {

		twitter->timeout_id = TT_TIMEOUT_STOPPED;

		return TRUE;

	}

	g_warning ( "Failed to remove timeout id:%i", twitter->timeout_id );

	return FALSE;
}


// Inicia un el timeout de actualizacion
gboolean tt_timeout_start (TwiTux *twitter)
{
	gboolean reload;

	twitux_conf_get_bool (twitux_conf_get (),
						  TWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
						  &reload);

	if (reload) {
		twitter->timeout_id = g_timeout_add (TT_TIMEOUT_STEP, tt_update_timeout, twitter);

		return TRUE;
	}
	return FALSE;
}


gboolean tt_network_get_data ( TwiTux *twitter, const gchar *url, SoupMessageCallbackFn callback, gboolean flag_current )
{
	SoupMessage *msg;
	gchar *purl;

	if ( twitter->processing ) return FALSE;

	// Cerrar todas las conexiones
	soup_session_abort ( twitter->conexion );

	tt_set_networking ( twitter, TRUE );

	purl = g_strdup ( url );

	if ( flag_current ){

		if ( twitter->current_timeline ){

			g_free ( twitter->current_timeline );

		}

		twitter->current_timeline = purl;
	}

	msg = soup_message_new ( "GET", purl );

	soup_session_queue_message (twitter->conexion, msg, callback, twitter);

	if ( !flag_current ) g_free ( purl );

	return TRUE;
}


gboolean tt_network_post_data ( TwiTux *twitter, const gchar *url, gchar *formdata, SoupMessageCallbackFn callback )
{
	SoupMessage *msg;

	tt_set_networking ( twitter, TRUE );

	msg = soup_message_new ( "POST", url );

	soup_message_add_header ( msg->request_headers, "X-Twitter-Client", TWITTER_HEADER_CLIENT );
	soup_message_add_header ( msg->request_headers, "X-Twitter-Client-Version", TWITTER_HEADER_VERSION );
	soup_message_add_header ( msg->request_headers, "X-Twitter-Client-URL", TWITTER_HEADER_URL );

	soup_message_set_request (msg, "application/x-www-form-urlencoded", SOUP_BUFFER_USER_OWNED, formdata, strlen ( formdata ) );

	soup_session_queue_message ( twitter->conexion, msg, callback, twitter );

	return TRUE;
}


gchar * tt_network_save_data ( TwiTux *twitter, SoupMessage *msg, const char *file )
{
	gchar *tmp_file = NULL;

	// Archivo de lista de amigos
	tmp_file = g_build_filename ( twitter->cache_files_dir, G_DIR_SEPARATOR_S, file, NULL );

	// Borro si existe
	tt_unlink ( tmp_file );

	// Guardo
	g_file_set_contents ( tmp_file, msg->response.body, msg->response.length , NULL );

	return tmp_file;
}


gboolean tt_network_check_status ( TwiTux *twitter, SoupMessage *msg )
{
	if ( msg->status_code == 1 ) {

		tt_cambiar_mensaje_estado ( twitter, _("Stopped.") );

		return FALSE;

	} else if ( msg->status_code == 401 ){

		tt_cambiar_mensaje_estado ( twitter, _("Access denied. Please login.") );

		return FALSE;

	} else if ( msg->status_code != 200 ){

		tt_cambiar_mensaje_estado ( twitter, _("HTTP communication error.") );

		return FALSE;

	}
	
	return TRUE;
}


gboolean tt_update_timeout ( gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	// Se cumple en casos especiales, como posteando o esperando
	// el timeline actual desde la ultima vez llamo tt_update_timeout
	if ( twitter->processing ) return TRUE;

	tt_network_get_timeline ( twitter, twitter->current_timeline, FALSE );

	twitter->timeout_id = TT_TIMEOUT_STOPPED;

	return FALSE;

}
