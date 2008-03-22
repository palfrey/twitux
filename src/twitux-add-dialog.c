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
 */

#include "config.h"

#include <gtk/gtkbuilder.h>
#include <gtk/gtkdialog.h>

#include <libtwitux/twitux-paths.h>

#include "twitux.h"
#include "twitux-add-dialog.h"
#include "twitux-network.h"

#define XML_FILE "add_friend_dlg.xml"

typedef struct {
	GtkWidget *entry;
	GtkWidget *dialog;
} TwituxAdd;

static void add_response_cb (GtkWidget *widget,
							 gint       response,
							 TwituxAdd *add);
static void add_destroy_cb  (GtkWidget *widget,
							 TwituxAdd *add);

static void
add_response_cb (GtkWidget     *widget,
				 gint           response,
				 TwituxAdd     *add)
{
	if (response == GTK_RESPONSE_OK) {
		twitux_network_add_user (gtk_entry_get_text (GTK_ENTRY (add->entry)));
	}
	gtk_widget_destroy (widget);
}

static void
add_destroy_cb (GtkWidget     *widget,
				TwituxAdd     *add)
{
	g_free (add);
}

void
twitux_add_dialog_show (GtkWindow *parent)
{
	static TwituxAdd *add;
	GtkBuilder       *ui;
	gchar            *path;
	GError           *err = NULL;

	if (add) {
		gtk_window_present (GTK_WINDOW (add->dialog));
		return;
	}

	add = g_new0 (TwituxAdd, 1);

	/* Create the gtkbuild and load the xml file */
	ui = gtk_builder_new ();
	gtk_builder_set_translation_domain (ui, GETTEXT_PACKAGE);
	path = twitux_paths_get_glade_path (XML_FILE);
	gtk_builder_add_from_file (ui, path, &err);
	g_free (path);

	/* Grab the widgets */
	add->dialog = GTK_WIDGET (gtk_builder_get_object (ui, "add_friend_dialog"));
	add->entry = GTK_WIDGET (gtk_builder_get_object (ui, "frienduser_entry"));
	
	/* Connect the signals */
	g_signal_connect (G_OBJECT (add->dialog), "destroy",
					  G_CALLBACK (add_destroy_cb),
					  add);
	g_signal_connect (G_OBJECT (add->dialog), "response",
					  G_CALLBACK (add_response_cb),
					  add);

	/* Set the parent */
	g_object_add_weak_pointer (G_OBJECT (add->dialog), (gpointer) &add);
	gtk_window_set_transient_for (GTK_WINDOW (add->dialog), parent);

	/* Now that we're done setting up, let's show the widget */
	gtk_widget_show (add->dialog);
}
