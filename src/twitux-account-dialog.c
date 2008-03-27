/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007-2008 Brian Pepple <bpepple@fedoraproject.org>
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
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>

#include <libtwitux/twitux-conf.h>
#include <libtwitux/twitux-paths.h>
#ifdef HAVE_GNOME_KEYRING
#include <libtwitux/twitux-keyring.h>
#endif

#include "twitux.h"
#include "twitux-account-dialog.h"

#define XML_FILE "account_dlg.xml"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *auto_login;
	GtkWidget *show_password;
} TwituxAccount;

static void      account_response_cb          (GtkWidget         *widget,
											   gint               response,
											   TwituxAccount     *act);
static void      account_destroy_cb           (GtkWidget         *widget,
											   TwituxAccount     *act);
static void      account_show_password_cb     (GtkWidget         *widget,
											   TwituxAccount     *act);

static void
account_response_cb (GtkWidget     *widget,
					 gint           response,
					 TwituxAccount *act)
{
	if (response == GTK_RESPONSE_OK) {
		TwituxConf *conf;

		conf = twitux_conf_get ();

		twitux_conf_set_string (conf,
								TWITUX_PREFS_AUTH_USER_ID,
								gtk_entry_get_text (GTK_ENTRY (act->username)));

#ifdef HAVE_GNOME_KEYRING
		twitux_keyring_set_password (gtk_entry_get_text (GTK_ENTRY (act->username)),
									 gtk_entry_get_text (GTK_ENTRY (act->password)));
#else
		twitux_conf_set_string (conf,
								TWITUX_PREFS_AUTH_PASSWORD,
								gtk_entry_get_text (GTK_ENTRY (act->password)));
#endif

		twitux_conf_set_bool (conf,
							  TWITUX_PREFS_AUTH_AUTO_LOGIN,
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (act->auto_login)));
	}
	gtk_widget_destroy (widget);
}

static void
account_destroy_cb (GtkWidget     *widget,
					TwituxAccount *act)
{
	g_free (act);
}

static void
account_show_password_cb (GtkWidget     *widget,
						  TwituxAccount *act)
{
	gboolean visible;

	visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (act->show_password));
	gtk_entry_set_visibility (GTK_ENTRY (act->password), visible);
}

void
twitux_account_dialog_show (GtkWindow *parent)
{
	static TwituxAccount *act;
	GtkBuilder           *ui;
	TwituxConf           *conf;
	gchar                *username;
	gchar                *password;
	gchar                *path;
	guint                 result;
	gboolean              login;
	GError               *err = NULL;

	if (act) {
		gtk_window_present (GTK_WINDOW (act->dialog));
		return;
	}

	act = g_new0 (TwituxAccount, 1);

	/* Create the gtkbuild and load the xml file */
	ui = gtk_builder_new ();
	gtk_builder_set_translation_domain (ui, GETTEXT_PACKAGE);
	path = twitux_paths_get_glade_path (XML_FILE);
	result = gtk_builder_add_from_file (ui, path, &err);
	g_free (path);

	if (result == 0) {
		g_warning ("Unable to get xml file: %s", err->message);
		g_error_free (err);
		return;
	}

	/* Grab the widgets */
	act->dialog = GTK_WIDGET (gtk_builder_get_object (ui, "account_dialog"));
	act->username = GTK_WIDGET (gtk_builder_get_object (ui, "username_entry"));
	act->password = GTK_WIDGET (gtk_builder_get_object (ui, "password_entry"));
	act->auto_login =
		GTK_WIDGET (gtk_builder_get_object (ui, "login_checkbutton"));
	act->show_password =
		GTK_WIDGET (gtk_builder_get_object (ui, "show_password_checkbutton"));

	/* Connect the signals */
	g_signal_connect (G_OBJECT (act->dialog), "destroy",
					  G_CALLBACK (account_destroy_cb),
					  act);
	g_signal_connect (G_OBJECT (act->dialog), "response",
					  G_CALLBACK (account_response_cb),
					  act);
	g_signal_connect (G_OBJECT (act->show_password), "toggled",
					  G_CALLBACK (account_show_password_cb),
					  act);

	g_object_add_weak_pointer (G_OBJECT (act->dialog), (gpointer) &act);

	gtk_window_set_transient_for (GTK_WINDOW (act->dialog), parent);

	/*
	 * Check to see if the username & pasword are already in gconf,
	 * and if so fill in the appropriate entry widget.
	 */
	conf = twitux_conf_get ();
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_USER_ID,
							&username);
	gtk_entry_set_text (GTK_ENTRY (act->username), username ? username : "");

#ifdef HAVE_GNOME_KEYRING
	/* If there is no username, don't bother searching for the password */
	if (G_STR_EMPTY (username)) {
		username = NULL;
		password = NULL;
	} else {
		if (!(twitux_keyring_get_password (username, &password))) {
			password = NULL;
		}
	}
#else
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_PASSWORD,
							&password);
#endif

	gtk_entry_set_text (GTK_ENTRY (act->password), password ? password : "");
	g_free (username);
	g_free (password);

	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_AUTH_AUTO_LOGIN,
						  &login);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (act->auto_login), login);

	/* Ok, let's go ahead and show it */
	gtk_widget_show (act->dialog);
}
