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

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtkcelllayout.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtktogglebutton.h>

#include <libtwitux/twitux-conf.h>
#include <libtwitux/twitux-xml.h>
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
	GtkWidget *service;
} TwituxAccount;

enum {
	COL_COMBO_VISIBLE_NAME,
	COL_COMBO_NAME,
	COL_COMBO_COUNT
};

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
		TwituxConf   *conf;
		gchar        *name;
		GtkTreeModel *model;
		GtkTreeIter   iter;

		conf = twitux_conf_get ();

		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (act->service), &iter)) {
			model = gtk_combo_box_get_model (GTK_COMBO_BOX (act->service));

			gtk_tree_model_get (model, &iter,
								COL_COMBO_NAME, &name,
								-1);

			twitux_conf_set_string (conf,
									TWITUX_PREFS_AUTH_SERVICE,
									name);
			g_free (name);
		}

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

static void
account_services_setup (TwituxAccount *act)
{
	static const gchar *services[] = {
		SERVICE_TWITTER, N_("Twitter"),
		SERVICE_IDENTICA, N_("Identi.ca"),
		NULL
	};

	GtkListStore    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	gint             i;

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (act->service),
								renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (act->service),
									renderer, "text", COL_COMBO_VISIBLE_NAME, NULL);

	model = gtk_list_store_new (COL_COMBO_COUNT,
								G_TYPE_STRING,
								G_TYPE_STRING);

	gtk_combo_box_set_model (GTK_COMBO_BOX (act->service),
							 GTK_TREE_MODEL (model));

	for (i = 0; services[i]; i += 2) {
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
							COL_COMBO_VISIBLE_NAME, _(services[i + 1]),
							COL_COMBO_NAME, services[i],
							-1);
	}

	g_object_unref (model);
}

static void
account_get_service (TwituxAccount *act)
{
	gchar        *service;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      found;

	if (!twitux_conf_get_string (twitux_conf_get (),
								 TWITUX_PREFS_AUTH_SERVICE,
								 &service)) {
		return;
	}

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (act->service));

	found = FALSE;
	if (service && gtk_tree_model_get_iter_first (model, &iter)) {
		gchar *name;

		do {
			gtk_tree_model_get (model, &iter,
								COL_COMBO_NAME, &name,
								-1);

			if (strcmp (name, service) == 0) {
				found = TRUE;
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (act->service), &iter);
				break;
			} else {
				found = FALSE;
			}
			g_free (name);
		} while (gtk_tree_model_iter_next (model, &iter));
	}

	/* Fallback to first one */
	if (!found) {
		if (gtk_tree_model_get_iter_first (model, &iter)) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (act->service), &iter);
		}
	}
	
	g_free (service);
}

void
twitux_account_dialog_show (GtkWindow *parent)
{
	static TwituxAccount *act;
	GtkBuilder           *ui;
	TwituxConf           *conf;
	gchar                *username;
	gchar                *password;
	gboolean              login;

	if (act) {
		gtk_window_present (GTK_WINDOW (act->dialog));
		return;
	}

	act = g_new0 (TwituxAccount, 1);

	/* Get widgets */
	ui = twitux_xml_get_file (XML_FILE,
							  "account_dialog", &act->dialog,
							  "username_entry", &act->username,
							  "password_entry", &act->password,
							  "login_checkbutton", &act->auto_login,
							  "show_password_checkbutton", &act->show_password,
							  "combobox_service", &act->service,
							  NULL);

	/* Connect the signals */
	twitux_xml_connect (ui, act,
						"account_dialog", "destroy", account_destroy_cb,
						"account_dialog", "response", account_response_cb,
						"show_password_checkbutton", "toggled", account_show_password_cb,
						NULL);

	g_object_unref (ui);

	g_object_add_weak_pointer (G_OBJECT (act->dialog), (gpointer) &act);

	gtk_window_set_transient_for (GTK_WINDOW (act->dialog), parent);

	/* Set up services combobox */
	account_services_setup (act);

	/* Set the combobox value from gconf setting */
	account_get_service (act);

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
