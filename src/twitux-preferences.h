/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 - Brian Pepple <bpepple@fedoraproject.org)
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

#ifndef __TWITUX_PREFERENCES_H__
#define __TWITUX_PREFERENCES_H__

G_BEGIN_DECLS

#define TWITUX_PREFS_PATH "/apps/twitux"

#define TWITUX_PREFS_AUTH_USER_ID             TWITUX_PREFS_PATH "/auth/user_id"
#define TWITUX_PREFS_AUTH_PASSWORD            TWITUX_PREFS_PATH "/auth/password"
#define TWITUX_PREFS_AUTH_REMEMBER            TWITUX_PREFS_PATH "/auth/remember_login"

#define TWITUX_PREFS_TWEETS_HOME_TIMELINE     TWITUX_PREFS_PATH "/tweets/home_timeline"
#define TWITUX_PREFS_TWEETS_RELOAD_TIMELINES  TWITUX_PREFS_PATH "/tweets/reload_timeline"

#define TWITUX_PREFS_UI_WINDOW_HEIGHT         TWITUX_PREFS_PATH "/ui/main_window_height"
#define TWITUX_PREFS_UI_WINDOW_WIDTH          TWITUX_PREFS_PATH "/ui/main_window_width"
#define TWITUX_PREFS_UI_WIN_POS_X             TWITUX_PREFS_PATH "/ui/main_window_pos_x"
#define TWITUX_PREFS_UI_WIN_POS_Y             TWITUX_PREFS_PATH "/ui/main_window_pos_y"
#define TWITUX_PREFS_UI_EXPAND_MESSAGES       TWITUX_PREFS_PATH "/ui/expand_messages"
#define TWITUX_PREFS_UI_NOTIFICATION          TWITUX_PREFS_PATH "/ui/notify"
#define TWITUX_PREFS_UI_STATUSBAR             TWITUX_PREFS_PATH "/ui/statusbar"

G_END_DECLS

#endif /* __TWITUX_PREFERENCES_H__ */
