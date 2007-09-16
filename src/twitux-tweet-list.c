/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Brian Pepple <bpepple@fedoraproject.org>
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

#include <gtk/gtk.h>

#include <libtwitux/twitux-debug.h>

#include "twitux.h"
#include "twitux-tweet-list.h"

#define DEBUG_DOMAIN "TweetList"

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TWITUX_TYPE_TWEET_LIST, TwituxTweetListPriv))

struct _TwituxTweetListPriv {
	GtkListStore *store;
};

static void   twitux_tweet_list_class_init (TwituxTweetListClass *klass);
static void   twitux_tweet_list_init       (TwituxTweetList      *tweet);
static void   tweet_list_finalize          (GObject              *obj);
static void   tweet_list_create_model      (TwituxTweetList      *list);
static void   tweet_list_setup_view        (TwituxTweetList      *list);

static TwituxTweetList *list = NULL;

G_DEFINE_TYPE (TwituxTweetList, twitux_tweet_list, GTK_TYPE_TREE_VIEW);

static void
twitux_tweet_list_class_init (TwituxTweetListClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = tweet_list_finalize;

	g_type_class_add_private (object_class, sizeof (TwituxTweetListPriv));
}

static void
twitux_tweet_list_init (TwituxTweetList *tweet)
{
	TwituxTweetListPriv *priv;

	list = tweet;

	priv = GET_PRIV (list);

	tweet_list_create_model (list);
	tweet_list_setup_view (list);

	/* TODO: Add signals */
}

static void
tweet_list_finalize (GObject *object)
{
	TwituxTweetListPriv *priv;

	priv = GET_PRIV (object);

	g_object_unref (priv->store);

	G_OBJECT_CLASS (twitux_tweet_list_parent_class)->finalize (object);
}

static void
tweet_list_create_model (TwituxTweetList *list)
{
	TwituxTweetListPriv *priv;
	GtkTreeModel        *model;

	priv = GET_PRIV (list);

	if (priv->store) {
		g_object_unref (priv->store);
	}

	priv->store = gtk_list_store_new (N_COLUMNS,
									  GDK_TYPE_PIXBUF,  /* Avatar pixbuf */
									  G_TYPE_STRING);   /* Tweet string */

	/* save normal model */
	model = GTK_TREE_MODEL (priv->store);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
}

static void
tweet_list_setup_view (TwituxTweetList *list)
{
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*avatar_column;
	GtkTreeViewColumn   *tweet_column;

	g_object_set (list,
				  "rules-hint", TRUE,
				  "reorderable", FALSE,
				  "headers-visible", FALSE,
				  NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	avatar_column = gtk_tree_view_column_new_with_attributes (NULL,
															  renderer,
															  "pixbuf", PIXBUF_AVATAR,
															  NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), avatar_column);

	renderer = gtk_cell_renderer_text_new ();
	tweet_column = gtk_tree_view_column_new_with_attributes (NULL,
															 renderer,
															 "markup", STRING_TEXT,
															 NULL);

	g_object_set (renderer,
				  "ypad", 0,
				  "xpad", 5,
				  "yalign", 0.0,
				  "wrap-mode", PANGO_WRAP_WORD_CHAR,
				  "wrap-width", 257, /* TODO: Have this set based on window geometry */
				  NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (list), tweet_column);
}

TwituxTweetList *
twitux_tweet_list_new (void)
{
	return g_object_new (TWITUX_TYPE_TWEET_LIST, NULL);
}

GtkListStore *
twitux_tweet_list_get_store (void)
{
	TwituxTweetListPriv *priv;
	
	priv = GET_PRIV (list);
	
	return priv->store;
}
