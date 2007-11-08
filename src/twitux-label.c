/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Daniel Morales <daniminas@gmail.com>
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
#include <stdlib.h>
#include <libsexy/sexy-url-label.h>
#include <libgnomevfs/gnome-vfs.h>

#include "twitux-label.h"
#include "twitux-network.h"

#define ACCEPT_LETTER_URL "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;/?@&=+$,-_.!~*'%"

#define ACCEPT_LETTER_USER "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"

static void   twitux_label_class_init   (TwituxLabelClass *clas);
static void   twitux_label_init         (TwituxLabel      *label);
static void   twitux_label_finalize     (GObject          *object);

static void  label_url_activated_cb     (GtkWidget        *url_label,
                                         gchar            *url,
                                         gpointer          user_data);
static char* label_parse_urls_alloc     (const char       *message);

G_DEFINE_TYPE (TwituxLabel, twitux_label, SEXY_TYPE_URL_LABEL);

static void
twitux_label_class_init (TwituxLabelClass *clas)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (clas);

	/* Add private */

	object_class->finalize = twitux_label_finalize;
}

static void
twitux_label_init (TwituxLabel *label)
{
	g_signal_connect (label,
			  "url-activated",
			  G_CALLBACK (label_url_activated_cb),
			  NULL);
}

static void
twitux_label_finalize (GObject *object)
{
	/* Cleanup code */
}

static void
label_url_activated_cb  (GtkWidget *url_label,
			 gchar     *url,
			 gpointer   user_data)
{
	GnomeVFSResult   result;

	/* Open an URL */
	if (!strncmp (url, "http://", 7) ||
		!strncmp (url, "ftp://", 6)) 
	{
		result = gnome_vfs_url_show (url);
		if (result != GNOME_VFS_OK) {
			g_warning ("Couldn't show URL: '%s'", url);
		}
	} else {
		twitux_network_get_user (url);
	}
}

TwituxLabel *
twitux_label_new (void)
{
	return g_object_new (TWITUX_TYPE_LABEL, NULL);
}

void
twitux_label_set_text (TwituxLabel  *nav,
					   const gchar  *text)
{
	gchar *parsed_text;
	
	if (!text)
		return;
	
	parsed_text = label_parse_urls_alloc (text);
	
	sexy_url_label_set_markup (SEXY_URL_LABEL (nav), parsed_text);
	
	g_free (parsed_text);
}

static char*
label_create_href_alloc ( const char* url )
{
	return g_strdup_printf ("<a href='%s'>%s</a>", url, url);
}


/* FIXME: Review this function (?) */
static char*
label_parse_urls_alloc (const char* message)
{
	const char* ptr = message;
	const char* last = ptr;
	char* ret = NULL;
	int len = 0;

	while(*ptr) {
		gboolean nicktl = !(strncmp(ptr, "@", 1));

		if (!strncmp(ptr, "http://", 7)
			|| !strncmp(ptr, "ftp://", 6)
			|| nicktl) {
	
			char* link;
			char* tiny_url;
			const char* tmp;

			if ( nicktl ) {
				ptr++;
			}

			if (last != ptr) {
				len += (ptr-last);
				if (!ret) {
					ret = malloc(len+1);
					memset(ret, 0, len+1);
				} else ret = realloc(ret, len+1);
				strncat(ret, last, ptr-last);
			}
			tmp = ptr;

			if ( nicktl ) { /* Parsing nickname */
			  while(*tmp && strchr(ACCEPT_LETTER_USER, *tmp)) tmp++;
			} else { /* Parsing URL */
			  while(*tmp && strchr(ACCEPT_LETTER_URL, *tmp)) tmp++;
			}

			link = malloc(tmp-ptr+1);
			memset(link, 0, tmp-ptr+1);
			memcpy(link, ptr, tmp-ptr);

			tiny_url = label_create_href_alloc (link);

			if (tiny_url) {
				free(link);
				link = tiny_url;
			}

			len += strlen(link);

			if (!ret) {
				ret = malloc(len+1);
				memset(ret, 0, len+1);
			} else ret = realloc(ret, len+1);

			strcat(ret, link);
			free(link);
			ptr = last = tmp;
		} else {
			ptr++;
		}
	}
	if (last != ptr) {
		len += (ptr-last);
		if (!ret) {
			ret = malloc(len+1);
			memset(ret, 0, len+1);
		} else ret = realloc(ret, len+1);
		strncat(ret, last, ptr-last);
	}
	return ret;
}
