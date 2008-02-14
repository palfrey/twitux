/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007-2008 Daniel Morales <daniminas@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Daniel Morales <daniminas@gmail.com>
 *
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <libsexy/sexy-url-label.h>

#include "twitux.h"
#include "twitux-label.h"
#include "twitux-network.h"

#define WORD_URL     1

static void   twitux_label_class_init   (TwituxLabelClass *clas);
static void   twitux_label_init         (TwituxLabel      *label);
static void   twitux_label_finalize     (GObject          *object);

static void  label_url_activated_cb     (GtkWidget        *url_label,
                                         gchar            *url,
                                         gpointer          user_data);
static char* label_msg_get_string       (const char       * message);

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
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	g_object_set (label,
				  "xalign", 0.0,
				  "yalign", 0.0,
				  "xpad", 6,
				  "ypad", 4,
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
	/* Open an URL */
	if (!strncmp (url, "http://", 7) ||	!strncmp (url, "ftp://", 6)) 
	{
		if (g_app_info_launch_default_for_uri (url, NULL, NULL) == FALSE) {
			g_warning ("Couldn't show URL: '%s'", url);
		}
	} else {
		twitux_network_get_user (url);
	}
}

GtkWidget *
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
	
	parsed_text = label_msg_get_string (text);
	
	sexy_url_label_set_markup (SEXY_URL_LABEL (nav), parsed_text);
	
	g_free (parsed_text);
}

static char*
label_create_href_alloc (const char* url)
{
	return g_strdup_printf ("<a href='%s'>%s</a>", url, url);
}

static gint
url_check_word (char *word, int len)
{
#define D(x) (x), ((sizeof (x)) - 1)
	static const struct {
		const char *s;
		gint        len;
	}
	prefix[] = {
		{ D("ftp.") },
		{ D("www.") },
		{ D("ftp://") },
		{ D("http://") },
		{ D("https://") },
	};
#undef D

	gint 		i;
	
	for (i = 0; i < G_N_ELEMENTS(prefix); i++) {
		int l;

		l = prefix[i].len;
		if (len > l) {
			gint j;

			/* This is pretty much strncasecmp(). */
			for (j = 0; j < l; j++)	{
				unsigned char c = word[j];
				
				if (tolower(c) != prefix[i].s[j])
					break;
			}
			if (j == l)
				return WORD_URL;
		}
	}
	
	return 0;
}

static char*
label_msg_get_string (const char* message)
{
	gchar **tokens;
	gchar  *result;
	gchar  *temp;
	gint 	i;
	
	if (G_STR_EMPTY (message)) {
		return NULL;
	}
	
	/* surround urls with <a> markup so that sexy-url-label can link it */
	tokens = g_strsplit_set (message, " \t\n", 0);
	if (url_check_word (tokens[0], strlen (tokens[0])) == WORD_URL) {
		result = g_strdup_printf ("<a href=\"%s\">%s</a>", tokens[0], tokens[0]);
	} else {
		result = g_strdup (tokens[0]);
	}

	for (i = 1; tokens[i]; i++) {
		if (url_check_word (tokens[i], strlen (tokens[i])) == WORD_URL) {
			temp = g_strdup_printf ("%s <a href=\"%s\">%s</a>", result, tokens[i], tokens[i]);
			g_free (result);
			result = temp;
		} else {
			temp = g_strdup_printf ("%s %s", result, tokens[i]);
			g_free (result);
			result = temp;
		}
	}
	g_strfreev (tokens);
	
	return result;	
}
