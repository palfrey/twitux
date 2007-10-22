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

#ifndef __TWITUX_SEND_MESSAGE_DIALOG_H__
#define __TWITUX_SEND_MESSAGE_DILAOG_H__

#include <gtk/gtkwindow.h>

void twitux_send_message_dialog_show (GtkWindow   *parent,
									  gboolean     show_friends);
void twitux_message_set_followers    (GList       *followers);
void twitux_message_correct_word     (GtkWidget   *textview,
									  GtkTextIter  start,
									  GtkTextIter  end,
									  const gchar *new_word);

#endif /* __TWITUX_SEND_MESSAGE_DIALOG_H__ */
