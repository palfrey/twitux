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

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main.h"
#include "twitter.h"
#include "common.h"
#include "gcommon.h"
#include "gui.h"
#include "network.h"

// Tipos de Parsers
enum {
	TWITTER_PASER_TIMELINE,
	TWITTER_PASER_FRIENDS,
	TWITTER_PASER_MESSAGES
};

static void parsear_statuses ( xmlNode * a_node, TwiTux *twitter );
static void parsear_users ( xmlNode * a_node, TwiTux *twitter );

static void parsear_statuse ( xmlNode * a_node, TwiTuxStatus *status );
static void parsear_status_user ( xmlNode * a_node, TwiTuxUser *statu );

static gboolean parser_twitter ( const gchar *file, TwiTux *twitter, gint xml_type );

void check_view_bubble ( TwiTux *twitter, gboolean show_bubble, gint nTwitts, gint lastTweet );

void tt_free_user ( TwiTuxUser *user )
{
	if ( !user ) return;

	if ( user->id ) g_free ( user->id );

	if ( user->screen_name ) g_free ( user->screen_name );

	if ( user->name ) g_free ( user->name );

	if ( user->profile_image_url ) g_free ( user->profile_image_url );

	if ( user->profile_image_md5 ) g_free ( user->profile_image_md5 );

	g_free ( user );

}

void tt_free_status ( TwiTuxStatus *status )
{
	if ( !status ) return;

	if ( status->text ) g_free ( status->text );

	if ( status->id ) g_free ( status->id );

	if ( status->created_at ) g_free ( status->created_at );

	g_free ( status );
}


gboolean tt_twitter_load_messages ( const gchar *file, TwiTux *twitter )
{
	return parser_twitter ( file, twitter, TWITTER_PASER_MESSAGES );
}


gboolean tt_twitter_load_friends ( const gchar *file, TwiTux *twitter )
{
	return parser_twitter ( file, twitter, TWITTER_PASER_FRIENDS );
}


gboolean tt_twitter_load_timeline ( const gchar *file, TwiTux *twitter )
{
	return parser_twitter ( file, twitter, TWITTER_PASER_TIMELINE );
}


static void parsear_status_user ( xmlNode * a_node, TwiTuxUser *user )
{
	xmlNode *cur_node = NULL;
	xmlBufferPtr buffer = xmlBufferCreate ();

	for ( cur_node = a_node; cur_node; cur_node = cur_node->next ) {

		if ( cur_node->type == XML_ELEMENT_NODE ) {

			if ( xmlNodeBufGetContent ( buffer, cur_node ) == 0 ) {

				if ( g_str_equal ( cur_node->name, "id" ) ) {

					user->id = g_strdup ( (gchar *)xmlBufferContent ( buffer ) );

				} else if ( g_str_equal ( cur_node->name, "screen_name" ) ) {

					user->screen_name = g_strdup ( (gchar *)xmlBufferContent ( buffer ) );

				} else if ( g_str_equal ( cur_node->name, "name" ) ) {

					user->name = g_strdup ( (gchar *)xmlBufferContent ( buffer ) );

				} else if ( g_str_equal ( cur_node->name, "location" ) ) {

					// No lo so

				} else if ( g_str_equal ( cur_node->name, "description" ) ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "profile_image_url") ) {

					gchar *tmp = (gchar *)xmlBufferContent ( buffer );

					// Guardo un md5 que es el nombre de la imagen en cache

					user->profile_image_md5 = tt_md5 ( tmp , buffer->use );	

					user->profile_image_url = g_strdup ( tmp );

				} else if ( g_str_equal ( cur_node->name, "url" ) ) {

					// No lo uso

				} else if ( g_str_equal (cur_node->name, "protected") ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "status" ) ) {

					// No lo uso, solo viene cuando obtengo la lista de amigos.

				} else {

					g_warning ( "Parser:unknown user node %s => %s", cur_node->name, xmlBufferContent ( buffer ) );

				}

				xmlBufferEmpty ( buffer );

			}

		}

	}

}


static void parsear_statuse ( xmlNode * a_node, TwiTuxStatus *status )
{
	xmlNode *cur_node = NULL;
	xmlBufferPtr buffer = xmlBufferCreate ();

	for ( cur_node = a_node; cur_node; cur_node = cur_node->next ) {

		if ( cur_node->type == XML_ELEMENT_NODE ) {

			if ( xmlNodeBufGetContent ( buffer, cur_node ) == 0 ) {

				if ( g_str_equal ( cur_node->name, "created_at" ) ) {

					gchar *tmp = (gchar *)xmlBufferContent ( buffer );

					status->created_at = tt_get_time ( tmp );

				} else if ( g_str_equal ( cur_node->name, "text" ) ) {

					gchar *tmp = (gchar *)xmlBufferContent ( buffer );

					status->text = g_strdup ( tmp );

				} else if ( g_str_equal ( cur_node->name, "id" ) ) {

					status->id = g_strdup ( (gchar *)xmlBufferContent ( buffer ) );

				} else if ( g_str_equal ( cur_node->name, "sender_id" ) ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "recipient_id" ) ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "sender_screen_name" ) ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "recipient_screen_name" ) ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "sender" ) ) {

					status->user = g_new0 (TwiTuxUser, 1);

					parsear_status_user ( cur_node->children, status->user  );

				} else if ( g_str_equal (cur_node->name, "recipient") ) {

					// No lo uso

				} else if ( g_str_equal ( cur_node->name, "user" ) ) {

					status->user = g_new0 (TwiTuxUser, 1);
	
					parsear_status_user ( cur_node->children, status->user  );

				} else if ( g_str_equal ( cur_node->name, "source" ) ) {

					// No lo uso

				} else {

					g_warning ( "Parser:parsear_direct_message:unknown node: %s => %s", cur_node->name, xmlBufferContent ( buffer ) );

				}
				
				xmlBufferEmpty (buffer);
				
			}
			
		}
		
	}
	
}


static void parsear_statuses (xmlNode * a_node, TwiTux *twitter )
{
	TwiTuxStatus *status;

	xmlNode *cur_node = NULL;

	gboolean show_bubble = ( twitter->last_id > TT_NO_LAST_ID );

	gint nTwitts = 0;

	gint lastTweet = 0;

	gboolean count = TRUE;

	for ( cur_node = a_node; cur_node; cur_node = cur_node->next ) {

		if ( cur_node->type == XML_ELEMENT_NODE ) {

			if ( g_str_equal ( cur_node->name, "status" ) || g_str_equal ( cur_node->name, "direct_message" ) ){

				gint sid;

				gchar *tmp_md5;

				status = g_new0 (TwiTuxStatus, 1);

				parsear_statuse ( cur_node->children, status );

				// Lo agrego a la ventana
				tt_add_post ( twitter->principal, status );

				sid = atoi ( status->id );

				// El primer tweet que se parsea es el ultimo posteado
				if ( lastTweet == 0 ){

					lastTweet = sid;

				}

				// El ultimo tweet que se cargo
				if ( sid == twitter->last_id ) {

					count = FALSE;

				}

				// Cuento twitts nuevos
				if ( count ) {

					nTwitts++;

				}

				// Ruta a la imagen :)
				tmp_md5 = g_strdup ( status->user->profile_image_md5 );
				g_free ( status->user->profile_image_md5 );

				status->user->profile_image_md5 = g_build_filename ( twitter->cache_images_dir, G_DIR_SEPARATOR_S, tmp_md5, NULL );
				g_free ( tmp_md5 );

				// Si existe la imagen el el cache, la muestro..
				if ( tt_is_file_exist ( status->user->profile_image_md5  ) ) {

					tt_set_pic ( twitter->principal->lista_tree,  status, status->user->profile_image_md5 );

				} else { // .. sino la cargo

					status->tree = twitter->principal->lista_tree;

					tt_network_get_image ( twitter, status );

				}

			} else if ( g_str_equal ( cur_node->name, "statuses" ) ){

				parsear_statuses (cur_node->children, twitter );

			} else if ( g_str_equal ( cur_node->name, "direct-messages" ) ){

				parsear_statuses (cur_node->children, twitter );

			} else {

				g_warning ( "Parser:parsear_statuses:unknown node: %s", cur_node->name );

			}

		}

	}

	// Mostrar la burbuja de notificacion
	check_view_bubble ( twitter, show_bubble, nTwitts, lastTweet );

}


static void parsear_users ( xmlNode * a_node, TwiTux *twitter )
{
	xmlNode *cur_node = NULL;

	TwiTuxUser *user;

	for ( cur_node = a_node; cur_node; cur_node = cur_node->next ) {

		if ( cur_node->type == XML_ELEMENT_NODE ) {

			if ( g_str_equal ( cur_node->name, "user" ) ){

				user = g_new0 (TwiTuxUser, 1);

				parsear_status_user ( cur_node->children, user );

				// Lo agrego al menu :)
				tt_add_friend ( twitter, user );

			} else if ( g_str_equal ( cur_node->name, "users" ) ){

				parsear_users ( cur_node->children, twitter );

			} else {

				g_warning ( "Parser:parsear_users:unknown node: %s", cur_node->name );

			}

		}

	}

}


static gboolean parser_twitter ( const gchar *file, TwiTux *twitter, gint xml_type )
{
	// Para parseo
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	// Checkeo que exista el archivo
	if( !tt_is_file_exist ( file ) ){

		g_warning ( "parser_twitter:file not exists: %s", file );

		return FALSE;

	}

	// Leo el XML
	doc = xmlReadFile ( file, NULL, 0);

	if (doc == NULL) {

		g_warning ( "parser_twitter:xmlReadFile: %s", file );

		return FALSE;

	}

	// Preparo el parseo
	root_element = xmlDocGetRootElement ( doc );

	if (root_element == NULL) {

		g_warning ( "parser_twitter:xmlDocGetRootElement: %s", file );

		xmlFreeDoc ( doc );

		return FALSE;

	}
	
	switch (xml_type)
	{
		case TWITTER_PASER_TIMELINE:
		case TWITTER_PASER_MESSAGES:

			// Borro elementos anteriores de la lista
			tt_clear_list ( twitter->principal->lista_tree );

			// Hago el parseo
			parsear_statuses ( root_element, twitter );

		break;
		case TWITTER_PASER_FRIENDS:

			// Hago el parseo
			parsear_users ( root_element, twitter );

		break;
		default:

			g_warning ( "parser_twitter:unknown parser" );

			return FALSE;
	}	

	// Libero memoria
	xmlFreeDoc ( doc );
	xmlCleanupParser ();

	return TRUE;
}


void check_view_bubble ( TwiTux *twitter, gboolean show_bubble, gint nTwitts, gint lastTweet )
{
	GtkWidget *window;

	// No mostrar bubble su ventana está oculta o con foco
	window = twitter->principal->ventana;

	if ( !(GTK_WIDGET_VISIBLE ( window ) && gtk_window_is_active ( GTK_WINDOW ( window ) )) && twitter->gconf->ver_burbujas ) {

		// Mostrar la burbuja de notificacion solo si hay tweets nuevos
		if ( show_bubble && ( nTwitts > 0 ) && ( twitter->last_id != lastTweet ) ) {

			gchar *tmp;

			tmp = g_strdup_printf ( _("you have %i new tweet(s) to read!"), nTwitts );

			tt_gui_show_bubble ( twitter, tmp );

			g_free ( tmp );

		}

	}

	if ( lastTweet > 0 ) {

		twitter->last_id = lastTweet;

	}

}
