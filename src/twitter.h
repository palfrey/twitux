#if !defined TWITUX_TWITTER_H
#define TWITUX_TWITTER_H
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

#include <gtk/gtk.h>
 
#define TWITTER_PUBLIC_TIMELINE "http://twitter.com/statuses/public_timeline.xml"
#define TWITTER_FRIENDS_LIST "http://twitter.com/statuses/friends.xml"

#define TWITTER_FRIENDS_TIMELINE "http://twitter.com/statuses/friends_timeline.xml"
#define TWITTER_DIRECT_MESSAGES "http://twitter.com/direct_messages.xml"
#define TWITTER_MY_REPLIES "http://twitter.com/statuses/replies.xml"

#define TWITTER_POST_STATUS "http://twitter.com/statuses/update.xml"
#define TWITTER_SEND_MESSAGE "http://twitter.com/direct_messages/new.xml"

#define TWITTER_MY_TIMELINE "http://twitter.com/statuses/user_timeline.xml"
//#define TWITTER_STAUS_DESTROY "http://twitter.com/statuses/destroy/%s.xml" //Otro formato
//#define TWITTER_STAUS_SHOW "http://twitter.com/statuses/show/%s.xml"
#define TWITTER_USER_TIMELINE "http://twitter.com/statuses/user_timeline/%s.xml"
//#define TWITTER_USER_INFO "http://twitter.com/users/show/%s.xml" //Otro formato

#define TWITTER_TWITUX_TIMELINE "http://twitter.com/statuses/user_timeline/twitux.xml"

#define TWITTER_FRIEND_ADD "http://twitter.com/friendships/create/%s.xml"

#define TWITTER_HEADER_CLIENT PACKAGE_NAME
#define TWITTER_HEADER_VERSION PACKAGE_VERSION
#define TWITTER_HEADER_URL "http://twitux.sourceforge.net/client.xml"

#define TWITTER_ABOUT_URL "http://live.gnome.org/DanielMorales/Twitux"
#define TWITTER_ABOUT_BUGZILLA "http://sourceforge.net/tracker/?func=add&group_id=198704&atid=966542"

#define TINYURL_API_URL "http://tinyurl.com/api-create.php"

#define TT_STATUSES_X_PAGE 20

#include "main.h"


typedef struct _TwiTuxUser TwiTuxUser;

struct _TwiTuxUser
{
	gchar *id;

	gchar *screen_name;
	gchar *name;

	gchar *profile_image_url;
	gchar *profile_image_md5;

};


typedef struct _TwiTuxStatus TwiTuxStatus;

struct _TwiTuxStatus
{
	TwiTuxUser *user;
	
	gchar *text;
	gchar *id;
	
	gchar *created_at;
	
	GtkWidget *tree;
	
	GtkTreeIter iter;
};

#define TWITUX_USER(obj) ((TwiTuxUser *)obj)

#define TWITUX_STATUS(obj) ((TwiTuxStatus *)obj)

gboolean tt_twitter_load_timeline ( const gchar *file, TwiTux *twitter );

gboolean tt_twitter_load_friends ( const gchar *file, TwiTux *twitter );

gboolean tt_twitter_load_messages ( const gchar *file, TwiTux *twitter );

void tt_free_user ( TwiTuxUser *user );

void tt_free_status ( TwiTuxStatus *status );

#endif
