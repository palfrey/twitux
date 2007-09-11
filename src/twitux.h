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

#ifndef __TWITUX_H__
#define __TWITUX_H__

G_BEGIN_DECLS

#define G_STR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

/* Twitter Timelines */
#define TWITUX_API_TIMELINE_PUBLIC	"http://twitter.com/statuses/public_timeline.xml"
#define TWITUX_API_TIMELINE_FRIENDS	"http://twitter.com/statuses/friends_timeline.xml"
#define TWITUX_API_TIMELINE_MY		"http://twitter.com/statuses/user_timeline.xml"
#define TWITUX_API_TIMELINE_USER	"http://twitter.com/statuses/user_timeline/%s.xml"
#define TWITUX_API_TIMELINE_TWITUX	"http://twitter.com/statuses/user_timeline/twitux.xml"

/* Twitux GConf Keys */
#define TWITUX_PREFS_PATH "/apps/twitux"

#define TWITUX_PREFS_AUTH_USER_ID             TWITUX_PREFS_PATH "/auth/user_id"
#define TWITUX_PREFS_AUTH_PASSWORD            TWITUX_PREFS_PATH "/auth/password"
#define TWITUX_PREFS_AUTH_AUTO_LOGIN          TWITUX_PREFS_PATH "/auth/auto_login"

#define TWITUX_PREFS_TWEETS_HOME_TIMELINE     TWITUX_PREFS_PATH "/tweets/home_timeline"
#define TWITUX_PREFS_TWEETS_RELOAD_TIMELINES  TWITUX_PREFS_PATH "/tweets/reload_timeline"

#define TWITUX_PREFS_UI_WINDOW_HEIGHT         TWITUX_PREFS_PATH "/ui/main_window_height"
#define TWITUX_PREFS_UI_WINDOW_WIDTH          TWITUX_PREFS_PATH "/ui/main_window_width"
#define TWITUX_PREFS_UI_WIN_POS_X             TWITUX_PREFS_PATH "/ui/main_window_pos_x"
#define TWITUX_PREFS_UI_WIN_POS_Y             TWITUX_PREFS_PATH "/ui/main_window_pos_y"
#define TWITUX_PREFS_UI_WINDOW_HIDE           TWITUX_PREFS_PATH "/ui/hide"
#define TWITUX_PREFS_UI_EXPAND_MESSAGES       TWITUX_PREFS_PATH "/ui/expand_messages"
#define TWITUX_PREFS_UI_NOTIFICATION          TWITUX_PREFS_PATH "/ui/notify"
#define TWITUX_PREFS_UI_SPELL_LANGUAGES       TWITUX_PREFS_PATH "/ui/spell_checker_languages"
#define TWITUX_PREFS_UI_SPELL                 TWITUX_PREFS_PATH "/ui/spell"
#define TWITUX_PREFS_UI_STATUSBAR             TWITUX_PREFS_PATH "/ui/statusbar"

/* Proxy configuration */
#define TWITUX_PROXY                          "/system/http_proxy"
#define TWITUX_PROXY_USE                      TWITUX_PROXY "/use_http_proxy"
#define TWITUX_PROXY_HOST                     TWITUX_PROXY "/host"
#define TWITUX_PROXY_PORT                     TWITUX_PROXY "/port"
#define TWITUX_PROXY_USE_AUTH                 TWITUX_PROXY "/use_authentication"
#define TWITUX_PROXY_USER                     TWITUX_PROXY "/authentication_user"
#define TWITUX_PROXY_PASS                     TWITUX_PROXY "/authentication_password"

/* File storage */
#define TWITUX_DIRECTORY                      "twitux"
#define TWITUX_CACHE_FILE                     TWITUX_DIRECTORY "/parser.xml"
#define TWITUX_CACHE_USERS                    TWITUX_DIRECTORY "/userslist.xml"
#define TWITUX_CACHE_IMAGES                   TWITUX_DIRECTORY "/avatars"

/* Client information for Twitter servers */
#define TWITUX_HEADER_URL                     "http://twitux.sourceforge.net/client.xml"

/* ListStore columns */
enum {
	PIXBUF_AVATAR,
	STRING_TEXT,
	N_COLUMNS
};

G_END_DECLS

#endif /* __TWITUX_H__ */
