/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 - Daniel Morales <daniminas@gmail.com>
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

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libtwitux/twitux-debug.h>

#include "twitux.h"
#include "twitux-parser.h"
#include "twitux-app.h"

#define DEBUG_DOMAIN_SETUP       "Parser" 

typedef struct 
{
	TwituxUser	*user;
	gchar		*text;
	gchar		*created_at;
	gchar		*id;
} TwituxStatus;


static TwituxUser	*parser_twitux_node_user   (xmlNode     *a_node);
static TwituxStatus	*parser_twitux_node_status (xmlNode     *a_node);

static xmlDoc		*parser_twitux_parse       (const char  *cache_file,
												xmlNode    **first_element);

/* id of the newest tweet showed */
static gint			last_id = 0;

static xmlDoc*
parser_twitux_parse (const char  *cache_file,
					 xmlNode    **first_element)
{
	xmlDoc	*doc = NULL;
	xmlNode	*root_element = NULL;

	/* Check if file exists (and  if is a file) */
	if(!g_file_test (cache_file,
					 G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)){
		twitux_debug (DEBUG_DOMAIN_SETUP,
					  "file doesn't exists: %s",
					  cache_file);
		return NULL;
	}

	/* Read the XML */
	doc = xmlReadFile (cache_file, NULL, 0);
	if (doc == NULL) {
		twitux_debug (DEBUG_DOMAIN_SETUP,
					  "failed reading file: %s",
					  cache_file);
		return NULL;
	}

	/* Get first element */
	root_element = xmlDocGetRootElement (doc);
	if (root_element == NULL) {
		twitux_debug (DEBUG_DOMAIN_SETUP,
					  "failed getting first element of file: %s",
					  cache_file);
		xmlFreeDoc (doc);
		return NULL;
	} else {
		*first_element = root_element;
	}

	return doc;
}


/* Parse a user-list XML ( friends, followers,... ) */
GList *
twitux_parser_users_list (const gchar *cache_file_uri)
{
	xmlDoc		*doc = NULL;
	xmlNode		*root_element = NULL;
	xmlNode		*cur_node = NULL;

	GList		*friends = NULL;

	TwituxUser 	*user;

	/* parse the xml */
	doc = parser_twitux_parse (cache_file_uri, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return NULL;
	}

	/* get users */
	for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		if (g_str_equal (cur_node->name, "user")){
			/* parse user */
			user = parser_twitux_node_user (cur_node->children);
			/* add to list */
			friends = g_list_append (friends, user);
		} else if (g_str_equal (cur_node->name, "users")){
			cur_node = cur_node->children;
		}
	} /* End of loop */

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return friends;
}


/* Parse a xml user node. Ex: add/del users responses */
TwituxUser *
twitux_parser_single_user (const gchar *cache_file_uri)
{
	xmlDoc		*doc = NULL;
	xmlNode		*root_element = NULL;
	TwituxUser 	*user;
	
	/* parse the xml */
	doc = parser_twitux_parse (cache_file_uri, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return NULL;
	}

	if (g_str_equal (root_element->name, "user")) {
		user = parser_twitux_node_user (root_element->children);
	}

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();
	
	return user;
}


/* Parse a timeline XML file */
GtkListStore *
twitux_parser_timeline (const gchar *cache_file_uri)
{
	xmlDoc		    *doc = NULL;
	xmlNode		    *root_element = NULL;
	xmlNode		    *cur_node = NULL;

	GtkListStore 	*store = NULL;
	GtkTreeIter	     iter;

	TwituxStatus 	*status;

	/* Count new tweets */
	gboolean show_notification = (last_id > 0);
	gint nTwitts = 0;
	gint lastTweet = 0;
	gboolean count = TRUE;

	/* parse the xml */
	doc = parser_twitux_parse (cache_file_uri, &root_element);

	if (!doc) {
		xmlCleanupParser ();
		return NULL;
	}

	/* Create a ListStore */
	store = gtk_list_store_new (N_COLUMNS,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING);

	/* get tweets or direct messages */
	for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		/* Timelines and direct messages */
		if (g_str_equal (cur_node->name, "status") ||
		    g_str_equal (cur_node->name, "direct_message")) {
			gchar *tweet;
			gint sid;

			/* Parse node */
			status = parser_twitux_node_status (cur_node->children);

			sid = atoi (status->id);
			
			/* the first tweet parsed is the 'newest' */
			if (lastTweet == 0){
				lastTweet = sid;
			}

			/* Last tweet showed */
			if (sid == last_id) {
				count = FALSE;
			}

			/* Count new tweets */
			if (count) {
				nTwitts++;
			}

			/* Create string for text column */
			/* TODO: More formatting of string, and convert time */
			tweet = g_strconcat ("<b>", status->user->name, "</b> - ",
								 status->created_at, "\n", status->text, NULL);

			/* Append to ListStore */
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter,
								STRING_TEXT, tweet,
								-1);
			
			/* Free the text column string */
			g_free (tweet);

			/* Get Image */
			twitux_network_get_image (status->user->image_url,
									  status->user->screen_name,
									  iter,
									  store);

			/* Free struct */
			parser_free_user (status->user);
			if (status->text)
				g_free (status->text);
			if (status->created_at)
				g_free (status->created_at);
			if (status->id)
				g_free (status->id);
			
			g_free (status);

		} else if (g_str_equal (cur_node->name, "statuses") ||
			g_str_equal (cur_node->name, "direct-messages")) {

			cur_node = cur_node->children;
		}

	} /* end of loop */

	/* Show UI notification */
	if (show_notification && (nTwitts > 0) &&
				(last_id != lastTweet)) {
		twitux_app_show_notification (nTwitts);
	}

	/* Remember last id showed */
	if (lastTweet > 0) {
		last_id = lastTweet;
	}

	/* Free memory */
	xmlFreeDoc (doc);
	xmlCleanupParser ();

	return store;
}


static TwituxUser *
parser_twitux_node_user (xmlNode *a_node)
{
	xmlNode		   *cur_node = NULL;
	xmlBufferPtr	buffer;
	TwituxUser     *user;

	buffer = xmlBufferCreate ();
	user = g_new0 (TwituxUser, 1);

	/* Begin 'users' node loop */
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		if (xmlNodeBufGetContent (buffer, cur_node) != 0)
			continue;
		if (g_str_equal (cur_node->name, "screen_name" )) {
			const xmlChar *tmp;

			tmp = xmlBufferContent (buffer);
			user->screen_name = g_strdup ((const gchar *)tmp);

		} else if (g_str_equal (cur_node->name, "name" )) {
			const xmlChar *tmp;

			tmp = xmlBufferContent (buffer);
			user->name = g_strdup ((const gchar *)tmp);

		} else if (g_str_equal (cur_node->name, "profile_image_url")) {
			const xmlChar *tmp;

			tmp = xmlBufferContent (buffer);
			user->image_url = g_strdup ((const gchar *)tmp);

		}

		/* Free buffer content */
		xmlBufferEmpty (buffer);

	} /* End of loop */

	/* Free buffer pointer */
	xmlBufferFree (buffer);

	return user;
}


static TwituxStatus *
parser_twitux_node_status (xmlNode *a_node)
{
	xmlNode		   *cur_node = NULL;
	xmlBufferPtr	buffer;
	TwituxStatus   *status;

	buffer = xmlBufferCreate ();
	status = g_new0 (TwituxStatus, 1);

	/* Begin 'status' or 'direct-messages' loop */
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type != XML_ELEMENT_NODE)
			continue;
		if (xmlNodeBufGetContent (buffer, cur_node) != 0)
			continue;
		if (g_str_equal (cur_node->name, "created_at")) {
			const xmlChar *tmp;

			tmp = xmlBufferContent(buffer);
			/* TODO : Convert time to: 'time ago' */
			status->created_at = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "id")) {
			const xmlChar *tmp;

			tmp = xmlBufferContent(buffer);
			status->id = g_strdup ((const gchar *)tmp);
		} else if (g_str_equal (cur_node->name, "text")) {
			const xmlChar *msg;
			gchar         *tmp;

			msg = xmlBufferContent (buffer);

			tmp = g_locale_to_utf8 ((const gchar *)msg,
									-1,
									NULL,
									NULL,
									NULL);

			status->text = g_markup_escape_text (tmp,-1);

			g_free (tmp);

		} else if (g_str_equal(cur_node->name, "sender") ||
				   g_str_equal (cur_node->name, "user")) {

			status->user = parser_twitux_node_user (cur_node->children);
		}

		/* Free buffer content */
		xmlBufferEmpty (buffer);

	} /* End of loop */

	/* Free buffer pointer */
	xmlBufferFree (buffer);
	
	return status;
}


/* Free a user struct */
void
parser_free_user (TwituxUser *user)
{
	if (!user)
		return;

	if (user->screen_name)
		g_free (user->screen_name);
	if (user->name)
		g_free (user->name);
	if (user->image_url)
		g_free (user->image_url);

	g_free (user);
}


void
parser_reset_lastid ()
{
	last_id = 0;
}
