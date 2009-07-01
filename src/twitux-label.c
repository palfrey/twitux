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

#include <libtwitux/twitux-debug.h>
#include "twitux.h"
#include "twitux-label.h"
#include "twitux-network.h"

#define DEBUG_DOMAIN	  "Label"

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
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	
	g_object_set (label,
				  "xalign", 0.0,
				  "yalign", 0.0,
				  "xpad", 6,
				  "ypad", 4,
				  NULL);
	
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
	if (g_app_info_launch_default_for_uri (url, NULL, NULL) == FALSE) {
			g_warning ("Couldn't show URL: '%s'", url);
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

static guint
do_twitter(GString *s, guint start)
{
	gssize end = 0;
	gssize i;

	for (i = start+1; s->str[i]; ++i) {
		if (!(g_ascii_isalnum(s->str[i]) || s->str[i] == '_')) {
			end = i;
			break;
		}
	}
	
	if (end == 0) {
		char *name = g_strdup(&s->str[start+1]);
		g_string_truncate(s, start);
		g_string_append_printf (s, "<a href=\"http://twitter.com/%s\">@%s</a>",
							 name,
							 name);
		g_free(name);
		return s->len;
	} else {
		guint ret;
		char *name = (char*)g_malloc(end-start), *temp;
		g_strlcpy(name, &s->str[start+1], end-start);

		temp =
			g_strdup_printf ("<a href=\"http://twitter.com/%s\">@%s</a>",
							 name, name);
		free(name);
		g_string_erase(s, start, end-start);
		g_string_insert(s, start, temp);
		ret = start+strlen(temp);
		g_free(temp);
		return ret;
	}
}

static guint
do_hashtag(GString *s, guint start)
{
	gssize end = 0;
	gssize i;

	if (start!=0 && s->str[start-1]!=' ') // hashtags need a space (or beginning of tweet) before them
		return start;

	for (i = start+1; s->str[i]; ++i) {
		if (!(g_ascii_isalnum(s->str[i]) || s->str[i] == '_')) {
			end = i;
			break;
		}
	}
	
	if (end == 0) {
		char *name = g_strdup(&s->str[start+1]);
		if (strlen(name) != 0)
		{
			g_string_truncate(s, start);
			g_string_append_printf (s, "<a href=\"http://hashtags.org/tag/%s\">#%s</a>",
								 name,
								 name);
		}
		g_free(name);
		return s->len;
	} else {
		guint ret;
		if (end-start == 1)
			return start;
		char *name = (char*)g_malloc(end-start), *temp;
		g_strlcpy(name, &s->str[start+1], end-start);

		temp =
			g_strdup_printf ("<a href=\"http://hashtags.org/tag/%s\">#%s</a>",
							 name, name);
		free(name);
		g_string_erase(s, start, end-start);
		g_string_insert(s, start, temp);
		ret = start+strlen(temp);
		g_free(temp);
		return ret;
	}
}

static char *
full_url(const char *url)
{
	if (g_strstr_len(url, -1, "www.")==url) /* www. at beginning, so need to expand url */
		return g_strdup_printf("http://%s", url);
	else
		return g_strdup(url);
}


static guint
do_url(GString *s, guint start)
{
	gssize end = -1;
	gssize i;
	guint ret;
	char *url, *furl;

	for (i = start; s->str[i]; ++i) {
		if (g_ascii_isspace(s->str[i]) || s->str[i] == '(' || s->str[i] == ')') {
			end = i;
			break;
		}
	}

	if (end == -1) {
		url = g_strdup(&s->str[start]);
		furl = full_url(url);
		g_string_truncate(s, start);
		g_string_append_printf (s, "<a href=\"%s\">%s</a>",
								furl, url);
		ret = s->len;
	} else {
		char *temp;
		url = (char*)g_malloc(end-start+1);
		g_strlcpy(url, &s->str[start], end-start+1);
		g_assert(start!=end);
		furl = full_url(url);
	
		temp = g_strdup_printf ("<a href=\"%s\">%s</a>",
								furl, url);
		g_string_erase(s, start, end-start);
		g_string_insert(s, start, temp);
		ret = start+strlen(temp);
		g_free(temp);
	}

	g_free(url);
	g_free(furl);
	return ret;
}

static char*
label_msg_get_string (const char* message)
{
	gint 		i, j, pos;
	if (G_STR_EMPTY (message)) {
		return NULL;
	}
	
	/* TODO: Do we need to escape out <>&"' so that pango markup doesn't get confused? */
	
	char *fixed_msg = g_strdup(message);
	g_strdelimit(fixed_msg, "\t\n", ' ');

	GString *s = g_string_new(fixed_msg);
	g_free(fixed_msg);
	
#define D(x, func) (x), (sizeof (x)-1), 0, func
	struct {
		const char *s;
		guint len;
		gint place;
		guint (*func)(GString *s, guint start);
	}
	
	prefix[] = {
		{ D("@", do_twitter) },
		{ D("ftp.", do_url) },
		{ D("www.", do_url) },
		{ D("ftp://", do_url) },
		{ D("http://", do_url) },
		{ D("https://", do_url) },
		{ D("#", do_hashtag) },
	};
#undef D
	
	for (pos = 0; pos<s->len; pos++) {
		for (i = 0; i < G_N_ELEMENTS(prefix); i++) {
			/* Look for prefixes */
			if (prefix[i].s[prefix[i].place] == s->str[pos])
			{
				prefix[i].place++;
				if (prefix[i].place == prefix[i].len)
				{
					twitux_debug (DEBUG_DOMAIN, "Hit eos for %s (pointing at '%c')\n",prefix[i].s,s->str[pos-prefix[i].len+1]);
					pos = prefix[i].func(s, pos-prefix[i].len+1);
					for (j = 0; j < G_N_ELEMENTS(prefix); j++)
						prefix[j].place = 0;
					break;
				}
			}
			else
				prefix[i].place = 0;
		}
	}
	twitux_debug(DEBUG_DOMAIN, "Net result is '%s'\n",s->str);
	
	return g_string_free(s, FALSE);
}
