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

#ifndef __TWITUX_TWEET_LIST_H__
#define __TWITUX_TWEET_LIST_H__

#include <gtk/gtktreeview.h>

G_BEGIN_DECLS

/*
 * TwituxTweetList 
 */ 
#define TWITUX_TYPE_TWEET_LIST         (twitux_tweet_list_get_type ())
#define TWITUX_TWEET_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TWITUX_TYPE_TWEET_LIST, TwituxTweetList))
#define TWITUX_TWEET_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TWITUX_TYPE_TWEET_LIST, TwituxTweetListClass))
#define TWITUX_IS_TWEET_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TWITUX_TYPE_TWEET_LIST))
#define TWITUX_IS_TWEET_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TWITUX_TYPE_TWEET_LIST))
#define TWITUX_TWEET_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TWITUX_TYPE_TWEET_LIST, TwituxTweetListClass))

typedef struct _TwituxTweetList      TwituxTweetList;
typedef struct _TwituxTweetListClass TwituxTweetListClass;
typedef struct _TwituxTweetListPriv  TwituxTweetListPriv;

struct _TwituxTweetList {
	GtkTreeView            parent;
};

struct _TwituxTweetListClass {
	GtkTreeViewClass       parent_class;
};

TwituxTweetList *     twitux_tweet_list_new                (void);
GtkListStore *        twitux_tweet_list_get_store          (void);

G_END_DECLS

#endif /* __TWITUX_TWEET_LIST_H__ */
