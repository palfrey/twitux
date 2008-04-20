/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2008 Daniel Morales <daniminas@gmail.com>
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
 */

#include "config.h"

#include <gtk/gtkbuilder.h>
#include <gtk/gtkdialog.h>

#include <libtwitux/twitux-xml.h>

#include "twitux.h"
#include "twitux-lists-dialog.h"
#include "twitux-network.h"

#define XML_FILE "lists_dlg.xml"


enum {
	FOLLOWER_USER,
	FOLLOWER_NAME,
	FOLLOWER_POINTER
};

typedef struct {
	GtkWidget    *dialog;
	GtkTreeView  *following_list;
	
	GtkTreeModel *following_store;
} TwituxLists;

static void lists_response_cb (GtkWidget   *widget,
							   gint         response,
							   TwituxLists *lists);
static void lists_destroy_cb  (GtkWidget   *widget,
							   TwituxLists *lists);

static TwituxLists *lists;

static void
lists_response_cb (GtkWidget     *widget,
				   gint           response,
				   TwituxLists   *lists)
{
	gtk_widget_destroy (widget);
}

static void
lists_destroy_cb (GtkWidget    *widget,
				  TwituxLists  *lists)
{
	g_free (lists);
}

void
twitux_lists_dialog_load_lists (GList *users)
{
	TwituxUser  *user;
	GtkTreeIter  iter;
	GList       *list;

	for (list = users; list; list = list->next)
	{
		user = (TwituxUser *)list->data;
		gtk_list_store_append (GTK_LIST_STORE (lists->following_store), &iter);

		gtk_list_store_set (GTK_LIST_STORE (lists->following_store),
							&iter,
							FOLLOWER_USER, user->screen_name,
							FOLLOWER_NAME, user->name,
							FOLLOWER_POINTER, user,
							-1);
	}

	/* Enable window */
	gdk_window_set_cursor (GTK_WIDGET (lists->dialog)->window, NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (lists->dialog), TRUE);
}

void
twitux_lists_dialog_show (GtkWindow *parent)
{
	GtkBuilder *ui;
	GList      *users;

	if (lists) {
		gtk_window_present (GTK_WINDOW (lists->dialog));
		return;
	}

	lists = g_new0 (TwituxLists, 1);

	/* Get widgets */
	ui = twitux_xml_get_file (XML_FILE,
						"lists_dialog", &lists->dialog,
						"following_list", &lists->following_list,
						NULL);

	lists->following_store = gtk_tree_view_get_model (lists->following_list);

	/* Connect the signals */
	twitux_xml_connect (ui, lists,
						"lists_dialog", "destroy", lists_destroy_cb,
						"lists_dialog", "response", lists_response_cb,
						NULL);

	g_object_unref (ui);

	/* Set the parent */
	g_object_add_weak_pointer (G_OBJECT (lists->dialog), (gpointer) &lists);
	gtk_window_set_transient_for (GTK_WINDOW (lists->dialog), parent);

	/* Now that we're done setting up, let's show the widget */
	gtk_widget_show (lists->dialog);

	/* Load lists */
	users = twitux_network_get_friends ();
	if (users){
		twitux_lists_dialog_load_lists (users);
	} else {
		GdkCursor *cursor;
		/* Disable window while retrieving lists */
		cursor = gdk_cursor_new (GDK_WATCH);
		gdk_window_set_cursor (GTK_WIDGET (lists->dialog)->window, cursor);
		gtk_widget_set_sensitive (lists->dialog, FALSE);
	}
}
