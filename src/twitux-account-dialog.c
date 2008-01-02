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

#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>
#include <glade/glade.h>

#include <libtwitux/twitux-conf.h>
#ifdef HAVE_GNOME_KEYRING
#include <libtwitux/twitux-keyring.h>
#endif

#include "twitux.h"
#include "twitux-account-dialog.h"
#include "twitux-glade.h"

typedef struct {
	GtkWidget *dialog;
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *auto_login;
	GtkWidget *show_password;
} TwituxAccount;

static void      account_response_cb          (GtkWidget         *widget,
											   gint               response,
											   TwituxAccount     *account);
static void      account_destroy_cb           (GtkWidget         *widget,
											   TwituxAccount     *account);
static void      account_show_password_cb     (GtkWidget         *widget,
											   TwituxAccount     *account);

static void
account_response_cb (GtkWidget     *widget,
					 gint           response,
					 TwituxAccount *account)
{
	if (response == GTK_RESPONSE_OK) {
		TwituxConf *conf;

		conf = twitux_conf_get ();

		twitux_conf_set_string (conf,
								TWITUX_PREFS_AUTH_USER_ID,
								gtk_entry_get_text (GTK_ENTRY (account->username)));

#ifdef HAVE_GNOME_KEYRING
		twitux_keyring_set_password (gtk_entry_get_text (GTK_ENTRY (account->username)),
									 gtk_entry_get_text (GTK_ENTRY (account->password)));
#else
		twitux_conf_set_string (conf,
								TWITUX_PREFS_AUTH_PASSWORD,
								gtk_entry_get_text (GTK_ENTRY (account->password)));
#endif

		twitux_conf_set_bool (conf,
							  TWITUX_PREFS_AUTH_AUTO_LOGIN,
							  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (account->auto_login)));
	}
	gtk_widget_destroy (widget);
}

static void
account_destroy_cb (GtkWidget     *widget,
					TwituxAccount *account)
{
	g_free (account);
}

static void
account_show_password_cb (GtkWidget     *widget,
						  TwituxAccount *account)
{
	gboolean visible;

	visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (account->show_password));
	gtk_entry_set_visibility (GTK_ENTRY (account->password), visible);
}

void
twitux_account_dialog_show (GtkWindow *parent)
{
	static TwituxAccount *account;
	GladeXML             *glade;
	TwituxConf           *conf;
	gchar                *username;
	gchar                *password;
	gboolean              login;

	if (account) {
		gtk_window_present (GTK_WINDOW (account->dialog));
		return;
	}

	account = g_new0 (TwituxAccount, 1);

	glade = twitux_glade_get_file ("main.glade",
								   "account_dialog",
								   NULL,
								   "account_dialog", &account->dialog,
								   "username_entry", &account->username,
								   "password_entry", &account->password,
								   "login_checkbutton", &account->auto_login,
								   "show_password_checkbutton", &account->show_password,
								   NULL);

	twitux_glade_connect (glade,
						  account,
						  "account_dialog", "destroy", account_destroy_cb,
						  "account_dialog", "response", account_response_cb,
						  "show_password_checkbutton", "toggled", account_show_password_cb,
						  NULL);

	g_object_unref (glade);

	g_object_add_weak_pointer (G_OBJECT (account->dialog), (gpointer) &account);

	gtk_window_set_transient_for (GTK_WINDOW (account->dialog), parent);

	conf = twitux_conf_get ();

	/*
	 * Check to see if the username & pasword are already in gconf,
	 * and if so fill in the appropriate entry widget.
	 */
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_USER_ID,
							&username);
	gtk_entry_set_text (GTK_ENTRY (account->username), username ? username : "");

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

	gtk_entry_set_text (GTK_ENTRY (account->password), password ? password : "");
	g_free (username);
	g_free (password);

	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_AUTH_AUTO_LOGIN,
						  &login);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (account->auto_login), login);

	gtk_widget_show (account->dialog);
}
