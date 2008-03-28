/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2002-2007 Imendio AB
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
 *
 * Authors: Brian Pepple <bpepple@fedoraproject.org>
 *			Daniel Morales <daniminas@gmail.com>
 */

#include <config.h>
#include <sys/stat.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

#include <libtwitux/twitux-debug.h>
#include <libtwitux/twitux-conf.h>
#include <libtwitux/twitux-paths.h>
#ifdef HAVE_GNOME_KEYRING
#include <libtwitux/twitux-keyring.h>
#endif

#include "twitux.h"
#include "twitux-about.h"
#include "twitux-account-dialog.h"
#include "twitux-app.h"
#include "twitux-geometry.h"
#include "twitux-glade.h"
#include "twitux-hint.h"
#include "twitux-network.h"
#include "twitux-preferences.h"
#include "twitux-send-message-dialog.h"
#include "twitux-ui-utils.h"
#include "twitux-tweet-list.h"
#include "twitux-add-dialog.h"
#include "twitux-label.h"

#ifdef HAVE_DBUS
#include "twitux-dbus.h"
#endif

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TWITUX_TYPE_APP, TwituxAppPriv))

#define DEBUG_DOMAIN_SETUP       "AppSetup"
#define DEBUG_QUIT

struct _TwituxAppPriv {
	/* Main widgets */
	GtkWidget         *window;
	TwituxTweetList   *listview;
	GtkWidget         *statusbar;

	/* Widgets that are enabled when we're connected/disconnected */
	GList             *widgets_connected;
	GList             *widgets_disconnected;

	/* Timeline menu items */
	GtkWidget         *menu_public;
	GtkWidget         *menu_friends;
	GtkWidget         *menu_mine;
	GtkWidget         *menu_twitux;
	GtkWidget         *menu_direct_messages;
	GtkWidget         *menu_direct_replies;

	GtkWidget         *view_friends;

	/* Status Icon */
	GtkStatusIcon     *status_icon;

	/* Status Icon Popup Menu */
	GtkWidget         *popup_menu;
	GtkWidget         *popup_menu_show_app;

	/* Notify */
	NotifyNotification *notification;

	/* Misc */
	guint              size_timeout_id;
	gboolean           friends_loaded;
	
	/* Expand messages widgets */
	GtkWidget         *expand_box;
	GtkWidget         *expand_image;
	GtkWidget         *expand_title;
	GtkWidget         *expand_label;
};

static void	    twitux_app_class_init			 (TwituxAppClass        *klass);
static void     twitux_app_init			         (TwituxApp             *app);
static void     app_finalize                     (GObject               *object);
static void     app_restore_main_window_geometry (GtkWidget             *main_window);
static void     app_setup                        (void);
static gboolean app_main_window_quit_confirm 	 (TwituxApp             *app,
												  GtkWidget             *window);
static void     app_main_window_quit_confirm_cb	 (GtkWidget             *dialog,
												  gint                   response,
												  TwituxApp             *app);
static void     app_main_window_destroy_cb       (GtkWidget             *window,
												  TwituxApp             *app);
static gboolean app_main_window_delete_event_cb  (GtkWidget             *window,
												  GdkEvent              *event,
												  TwituxApp             *app);
static void     app_twitter_connect_cb           (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_twitter_disconnect_cb        (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_twitter_new_msg_cb           (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_twitter_send_msg_cb          (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_twitter_quit_cb              (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_twitter_refresh_cb           (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_settings_account_cb          (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_settings_preferences_cb       (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_view_public_timeline_cb      (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void     app_view_friends_timeline_cb     (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void     app_view_my_timeline_cb          (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void     app_view_direct_messages_cb      (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void     app_view_direct_replies_cb       (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void      app_view_twitux_timeline_cb     (GtkCheckMenuItem      *item,
												  TwituxApp             *app);
static void     app_view_select_friends          (GtkItem               *item,
												  TwituxApp             *app);
static void     app_help_about_cb                (GtkWidget             *window,
												  TwituxApp             *app);
static void     app_status_icon_activate_cb      (GtkStatusIcon         *status_icon,
												  TwituxApp             *app);
static void     app_status_icon_popup_menu_cb    (GtkStatusIcon         *status_icon,
												  guint                  button,
												  guint                  activate_time,
												  TwituxApp             *app);
static void     app_twitter_view_friend_cb       (GtkMenuItem           *menuitem,
												  TwituxUser            *user);
static void     app_twitter_add_friend_cb         (GtkMenuItem          *menuitem,
												  TwituxApp             *app);
static void     app_connection_items_setup       (GladeXML              *glade);
static void     app_login                        (void);
static void     app_retrieve_default_timeline    (void);
static void     app_status_icon_create_menu      (void);
static void     app_status_icon_create           (void);
static void     app_check_dir                    (void);
static gboolean configure_event_timeout_cb       (GtkWidget             *widget);
static gboolean app_window_configure_event_cb    (GtkWidget             *widget,
												  GdkEventConfigure     *event,
												  TwituxApp             *app);

static TwituxApp  *app = NULL;

G_DEFINE_TYPE (TwituxApp, twitux_app, G_TYPE_OBJECT);

static void
twitux_app_class_init (TwituxAppClass *klass)
{
	GObjectClass  *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = app_finalize;

	g_type_class_add_private (object_class, sizeof (TwituxAppPriv));
}

static void
twitux_app_init (TwituxApp *singleton_app)
{
	TwituxAppPriv *priv;

	app = singleton_app;

	priv = GET_PRIV (app);
}

static void
app_finalize (GObject *object)
{
	TwituxApp	       *app;
	TwituxAppPriv      *priv;	
	
	app = TWITUX_APP (object);
	priv = GET_PRIV (app);

	if (priv->size_timeout_id) {
		g_source_remove (priv->size_timeout_id);
	}

	g_list_free (priv->widgets_connected);
	g_list_free (priv->widgets_disconnected);

#ifdef HAVE_DBUS
	twitux_dbus_nm_finalize ();
#endif

	twitux_conf_shutdown ();
	
	G_OBJECT_CLASS (twitux_app_parent_class)->finalize (object);
}

static void
app_restore_main_window_geometry (GtkWidget *main_window)
{
	twitux_geometry_load_for_main_window (main_window);
}

static void
app_setup (void)
{
	TwituxAppPriv    *priv;
	TwituxConf		 *conf;
	GladeXML         *glade;
	GtkWidget        *scrolled_window;
	GtkWidget        *expand_vbox;
	gboolean          login;
	gboolean		  hidden;

	twitux_debug (DEBUG_DOMAIN_SETUP, "Beginning....");

	priv = GET_PRIV (app);

	/* Set up interface */
	twitux_debug (DEBUG_DOMAIN_SETUP, "Initialising interface");
	glade = twitux_glade_get_file ("main_window.glade",
								   "main_window",
								   NULL,
								   "main_window", &priv->window,
								   "main_scrolledwindow", &scrolled_window,
								   "main_statusbar", &priv->statusbar,
								   "view_public_timeline", &priv->menu_public,
								   "view_friends_timeline", &priv->menu_friends,
								   "view_my_timeline", &priv->menu_mine,
								   "view_twitux_timeline", &priv->menu_twitux,
								   "view_direct_messages", &priv->menu_direct_messages,
								   "view_friends", &priv->view_friends,
								   "expand_box", &priv->expand_box,
								   "expand_vbox", &expand_vbox,
								   "expand_image", &priv->expand_image,
								   "expand_title", &priv->expand_title,
								   NULL);

	twitux_glade_connect (glade,
						  app,
						  "main_window", "destroy", app_main_window_destroy_cb,
						  "main_window", "delete_event", app_main_window_delete_event_cb,
						  "main_window", "configure_event", app_window_configure_event_cb,
						  "twitter_connect", "activate", app_twitter_connect_cb,
						  "twitter_disconnect", "activate", app_twitter_disconnect_cb,
						  "twitter_new_message", "activate", app_twitter_new_msg_cb,
						  "twitter_send_direct_message", "activate", app_twitter_send_msg_cb,
						  "twitter_refresh", "activate", app_twitter_refresh_cb,
						  "twitter_quit", "activate", app_twitter_quit_cb,
						  "twitter_add_friend", "activate", app_twitter_add_friend_cb,
						  "settings_account", "activate", app_settings_account_cb,
						  "settings_preferences", "activate", app_settings_preferences_cb,
						  "view_public_timeline", "toggled", app_view_public_timeline_cb,
						  "view_friends_timeline", "toggled", app_view_friends_timeline_cb,
						  "view_my_timeline", "toggled", app_view_my_timeline_cb,
						  "view_direct_messages", "toggled", app_view_direct_messages_cb,
						  "view_direct_replies", "toggled", app_view_direct_replies_cb,
						  "view_twitux_timeline", "toggled", app_view_twitux_timeline_cb,
						  "view_friends", "select", app_view_select_friends,
						  "help_about", "activate", app_help_about_cb,
						  NULL);

	/* Set up connected related widgets */
	app_connection_items_setup (glade);
	g_object_unref (glade);

	/* Let's hide the main window, while we are setting up the ui */
	gtk_widget_hide (GTK_WIDGET (priv->window));

#ifdef HAVE_DBUS
	/* Initialize NM */
	twitux_dbus_nm_init ();
#endif

	/* Set-up the notification area */
	twitux_debug (DEBUG_DOMAIN_SETUP,
				  "Configuring notification area widget...");
	app_status_icon_create_menu ();
	app_status_icon_create ();
	
	/* Set the main window geometry */ 	 
	app_restore_main_window_geometry (priv->window);

	/* Set-up list view */
	priv->listview = twitux_tweet_list_new ();
	gtk_widget_show (GTK_WIDGET (priv->listview));
	gtk_container_add (GTK_CONTAINER (scrolled_window),
					   GTK_WIDGET (priv->listview));

	/* Set-up expand messages panel */
	priv->expand_label = twitux_label_new ();
	gtk_widget_show (GTK_WIDGET (priv->expand_label));
	gtk_box_pack_end (GTK_BOX (expand_vbox),
					   GTK_WIDGET (priv->expand_label),
					   TRUE, TRUE, 0);
	gtk_widget_hide (GTK_WIDGET (priv->expand_box));

	/* Initial status of widgets */
	twitux_app_state_on_connection (FALSE);

	/* Check Twitux directory and images cache */
	app_check_dir ();

	/* Assign the conf object to a variable since we're using it more than once */
	conf = twitux_conf_get ();
	
	/* Get the gconf value for whether the window should be hidden on start-up */
	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
						  &hidden);
	
	/* Ok, set the window state based on the gconf value */				  
	if (hidden) {
		gtk_widget_hide (priv->window);
	} else {
		gtk_widget_show (priv->window);
	}

	/*Check to see if we should automatically login */
	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_AUTH_AUTO_LOGIN,
						  &login);

	if (login) 
		app_login ();
}

static gboolean
app_main_window_quit_confirm (TwituxApp *app,
							  GtkWidget *window)
{
	TwituxAppPriv *priv;
	GtkWidget *dialog;
	
	priv = GET_PRIV (app);
	
	dialog = gtk_message_dialog_new_with_markup	(GTK_WINDOW (twitux_app_get_window ()),
												 0,
												 GTK_MESSAGE_WARNING,
												 GTK_BUTTONS_OK_CANCEL,
												 "<b>%s</b>",
												 _("Do you really want to quit?"));

	g_signal_connect (dialog, "response",
					  G_CALLBACK (app_main_window_quit_confirm_cb),
					  app);

	gtk_widget_show (dialog);

	return TRUE;
}

static void
app_main_window_quit_confirm_cb (GtkWidget *dialog,
								 gint       response,
								 TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	gtk_widget_destroy (dialog);

	if (response == GTK_RESPONSE_OK) {
		gtk_widget_destroy (priv->window);
	}
}

static void
app_main_window_destroy_cb (GtkWidget *window, TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	/* Add any clean-up code here */

#ifdef DEBUG_QUIT
	gtk_main_quit ();
#else
	exit (0);
#endif
}

static gboolean
app_main_window_delete_event_cb (GtkWidget *window,
								 GdkEvent  *event,
								 TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (gtk_status_icon_is_embedded (priv->status_icon)) {
		twitux_hint_show (TWITUX_PREFS_HINTS_CLOSE_MAIN_WINDOW,
						  _("Twitux is still running, it is just hidden."),
						  _("Click on the notification area icon to show Twitux."),
						   GTK_WINDOW (twitux_app_get_window ()),
						   NULL, NULL);
		
		twitux_app_set_visibility (FALSE);

		return TRUE;
	}

	if (app_main_window_quit_confirm (app, window)) {
		/* Don't quit if we have messages open */
		return TRUE;
	}
	
	if (twitux_hint_dialog_show (TWITUX_PREFS_HINTS_CLOSE_MAIN_WINDOW,
								_("You were about to quit!"),
								_("Since no system or notification tray has been "
								"found, this action would normally quit Twitux.\n\n"
								"This is just a reminder, from now on, Twitux will "
								"quit when performing this action unless you uncheck "
								"the option below."),
								GTK_WINDOW (twitux_app_get_window ()),
								NULL, NULL)) {
		/* Shown, we don't quit because the callback will
		 * decide that based on the YES|NO response from the
		 * question we are about to ask, since this behaviour
		 * is new.
		 */
		return TRUE;
	}

	/* At this point, we have checked we have:
	 *   - No tray
	 *   - Have NOT shown the hint
	 *
	 * So we just quit.
	 */

	return FALSE;
}

gboolean
twitux_app_is_window_visible (void)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	return twitux_window_get_is_visible (GTK_WINDOW (priv->window));
}

void
twitux_app_toggle_visibility (void)
{
	TwituxAppPriv *priv;
	gboolean       visible;

	priv = GET_PRIV (app);

	visible = twitux_window_get_is_visible (GTK_WINDOW (priv->window));

	if (visible && gtk_status_icon_is_embedded (priv->status_icon)) {
		gint x, y, w, h;

		gtk_window_get_size (GTK_WINDOW (priv->window), &w, &h);
		gtk_window_get_position (GTK_WINDOW (priv->window), &x, &y);

		twitux_geometry_save_for_main_window (x, y, w, h);

		if (priv->size_timeout_id) {
			g_source_remove (priv->size_timeout_id);
			priv->size_timeout_id = 0;
		}

		gtk_widget_hide (priv->window);

		twitux_conf_set_bool (twitux_conf_get (),
							  TWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
							  TRUE);
	} else {
		twitux_geometry_load_for_main_window (priv->window);
		twitux_window_present (GTK_WINDOW (priv->window), TRUE);

		twitux_conf_set_bool (twitux_conf_get (),
							  TWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
							  FALSE);
	}
}

void
twitux_app_set_visibility (gboolean visible)
{
	GtkWidget *window;

	window = twitux_app_get_window ();

	twitux_conf_set_bool (twitux_conf_get (),
						  TWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN,
						  !visible);

	if (visible) {
		twitux_window_present (GTK_WINDOW (window), TRUE);
	} else {
		gtk_widget_hide (window);
	}
}

static void
app_twitter_connect_cb (GtkWidget *widget,
						TwituxApp *app)
{
	app_login ();
}

static void
app_twitter_disconnect_cb (GtkWidget *widget,
						   TwituxApp *app)
{
	twitux_network_logout ();
	twitux_app_state_on_connection (FALSE);
}

static void
app_twitter_new_msg_cb (GtkWidget *widget,
						TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_send_message_dialog_show (GTK_WINDOW (priv->window));
	twitux_message_show_friends (FALSE);
}

static void
app_twitter_send_msg_cb (GtkWidget *widget,
						 TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_send_message_dialog_show (GTK_WINDOW (priv->window));
	twitux_message_show_friends (TRUE);
}

static void
app_twitter_quit_cb (GtkWidget  *widget,
					 TwituxApp  *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	gtk_widget_destroy (priv->window);
}

static void
app_twitter_refresh_cb (GtkWidget *window,
					    TwituxApp *app)
{
	twitux_network_refresh ();
}

static void
app_view_public_timeline_cb (GtkCheckMenuItem *item,
							 TwituxApp        *app)
{
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);

	if (!current)
		return;

	twitux_network_get_timeline (TWITUX_API_TIMELINE_PUBLIC);
}

static void
app_view_friends_timeline_cb (GtkCheckMenuItem *item,
							  TwituxApp        *app)
{
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);

	if (!current)
		return;

	twitux_network_get_timeline (TWITUX_API_TIMELINE_FRIENDS);
}

static void
app_view_my_timeline_cb (GtkCheckMenuItem *item,
						 TwituxApp        *app)
{
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);

	if (!current)
		return;

	twitux_network_get_user (NULL);
}

static void
app_view_direct_messages_cb (GtkCheckMenuItem *item,
							TwituxApp        *app)
{
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);

	if (!current)
		return;

	twitux_network_get_timeline (TWITUX_API_DIRECT_MESSAGES);
}

static void
app_view_direct_replies_cb (GtkCheckMenuItem *item,
							TwituxApp        *app)
{
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);

	if (!current)
		return;

	twitux_network_get_timeline (TWITUX_API_REPLIES);
}

static void
app_view_twitux_timeline_cb (GtkCheckMenuItem *item, 	 
	                         TwituxApp        *app) 	 
{ 	 
	gboolean  current;

	current = gtk_check_menu_item_get_active (item);
	  	 
	if (!current)
		return;
	  	 
	twitux_network_get_timeline (TWITUX_API_TIMELINE_TWITUX); 	 
}

static void
app_view_select_friends (GtkItem *item,
						 TwituxApp *app)
{
	GList *friends;

	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->friends_loaded)
		return;

	friends = twitux_network_get_friends ();
	
	if (friends) {
		twitux_app_set_friends (friends);
	}
}

static void
app_settings_account_cb (GtkWidget *widget,
						 TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_account_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_settings_preferences_cb (GtkWidget *widget,
							 TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_preferences_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_help_about_cb (GtkWidget *widget,
				   TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_about_dialog_new (GTK_WINDOW (priv->window));
}

static void
app_twitter_add_friend_cb (GtkMenuItem *menuitem,
						   TwituxApp *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	twitux_add_dialog_show (GTK_WINDOW (priv->window));
}

static void
app_show_hide_cb (GtkWidget *widget,
				  TwituxApp *app)
{
	twitux_app_toggle_visibility ();
}

static void
app_status_icon_activate_cb (GtkStatusIcon *status_icon,
							 TwituxApp     *app)
{
	twitux_app_toggle_visibility ();
}

static void
app_status_icon_popup_menu_cb (GtkStatusIcon *status_icon,
							   guint          button,
							   guint          activate_time,
							   TwituxApp     *app)
{
	TwituxAppPriv *priv;
	gboolean       show;

	priv = GET_PRIV (app);

	show = twitux_window_get_is_visible (GTK_WINDOW (priv->window));

	g_signal_handlers_block_by_func (priv->popup_menu_show_app,
									 app_show_hide_cb, app);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->popup_menu_show_app),
									show);
	g_signal_handlers_unblock_by_func (priv->popup_menu_show_app,
									   app_show_hide_cb, app);

	gtk_menu_popup (GTK_MENU (priv->popup_menu),
					NULL, NULL,
					gtk_status_icon_position_menu,
					priv->status_icon,
					button,
					activate_time);
}

static void
app_status_icon_create_menu (void)
{
	TwituxAppPriv *priv;
	GladeXML      *glade;

	priv = GET_PRIV (app);

	glade = twitux_glade_get_file ("tray_menu.glade",
								   "tray_menu",
								   NULL,
								   "tray_menu", &priv->popup_menu,
								   "tray_show_app", &priv->popup_menu_show_app,
								   NULL);

	twitux_glade_connect (glade,
						  app,
						  "tray_new_message", "activate", app_twitter_new_msg_cb,
						  "tray_quit", "activate", app_twitter_quit_cb,
						  NULL);

	g_signal_connect (priv->popup_menu_show_app,
					  "toggled",
					  G_CALLBACK (app_show_hide_cb), app);

	g_object_unref (glade);
}

static void
app_status_icon_create (void)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	priv->status_icon = gtk_status_icon_new_from_icon_name ("twitux");
	g_signal_connect (priv->status_icon,
					  "activate",
					  G_CALLBACK (app_status_icon_activate_cb),
					  app);

	g_signal_connect (priv->status_icon,
					  "popup_menu",
					  G_CALLBACK (app_status_icon_popup_menu_cb),
					  app);

	gtk_status_icon_set_visible (priv->status_icon, TRUE);
}

void
twitux_app_create (void)
{
	g_object_new (TWITUX_TYPE_APP, NULL);

	app_setup ();
}

TwituxApp *
twitux_app_get (void)
{
	g_assert (app != NULL);
	
	return app;
}
 
static gboolean
configure_event_timeout_cb (GtkWidget *widget)
{
	TwituxAppPriv *priv;
	gint           x, y, w, h;

	priv = GET_PRIV (app);

	gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
	gtk_window_get_position (GTK_WINDOW (widget), &x, &y);

	twitux_geometry_save_for_main_window (x, y, w, h);

	priv->size_timeout_id = 0;

	return FALSE;
}

static gboolean
app_window_configure_event_cb (GtkWidget         *widget,
							   GdkEventConfigure *event,
							   TwituxApp         *app)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	if (priv->size_timeout_id) {
		g_source_remove (priv->size_timeout_id);
	}

	priv->size_timeout_id = g_timeout_add (500,
										   (GSourceFunc) configure_event_timeout_cb,
										   widget);

	return FALSE;
}

static void
app_twitter_view_friend_cb (GtkMenuItem *menuitem,
							TwituxUser *user)
{
	twitux_network_get_user (user->screen_name);
}

static void
app_login (void)
{
	TwituxAppPriv *priv;
	TwituxConf    *conf;
	gchar         *username;
	gchar         *password;

#ifdef HAVE_DBUS
	gboolean connected = TRUE;

	/*
	 * Don't try to connect if we have
	 * Network Manager state and we are NOT connected.
	 */
	if (twitux_dbus_nm_get_state (&connected) && !connected) {
		return;
	}
#endif
	
	priv = GET_PRIV (app);

	conf = twitux_conf_get ();
	twitux_conf_get_string (conf,
							TWITUX_PREFS_AUTH_USER_ID,
							&username);

#ifdef HAVE_GNOME_KEYRING
	if (G_STR_EMPTY (username)) {
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

	if (G_STR_EMPTY (username) || G_STR_EMPTY (password)) {
		twitux_account_dialog_show (GTK_WINDOW (priv->window));
	} else {
		twitux_network_login ();
		app_retrieve_default_timeline ();
	}

	priv->friends_loaded = FALSE;

	g_free (username);
	g_free (password);
}

static void
app_retrieve_default_timeline (void)
{
	TwituxAppPriv *priv;
	gchar         *timeline;

	priv = GET_PRIV (app);

	twitux_conf_get_string (twitux_conf_get (),
							TWITUX_PREFS_TWEETS_HOME_TIMELINE,
							&timeline);

	/* This shouldn't happen, but just in case */
	if (G_STR_EMPTY (timeline)) {
		g_warning ("Default timeline in not set");
		timeline = g_strdup (TWITUX_API_TIMELINE_FRIENDS);
	}

	if (strcmp (timeline, TWITUX_API_TIMELINE_FRIENDS) == 0) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->menu_friends),
										TRUE);
	} else if (strcmp (timeline, TWITUX_API_TIMELINE_PUBLIC) == 0) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->menu_public),
										TRUE);
	} else if (strcmp (timeline, TWITUX_API_TIMELINE_MY) == 0) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->menu_mine),
										TRUE);
	} else if (strcmp (timeline, TWITUX_API_TIMELINE_TWITUX) == 0) {
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->menu_twitux),
										TRUE);
	} else {
		/* Let's fallback to friends timeline */
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->menu_friends),
										TRUE);
	}

	twitux_network_get_timeline (timeline);
  
	g_free (timeline);
}

static void
app_check_dir (void)
{
	gchar    *file;

	file = g_build_filename (g_get_home_dir (), ".gnome2", TWITUX_CACHE_IMAGES, NULL);

	if (!g_file_test (file, G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR)) {
		twitux_debug (DEBUG_DOMAIN_SETUP, "Making directory: %s", file);
		g_mkdir_with_parents (file, S_IRUSR|S_IWUSR|S_IXUSR);
	}

	g_free (file);
}

static void
app_connection_items_setup (GladeXML *glade)
{
	TwituxAppPriv *priv;

	const gchar   *widgets_connected[] = {
		"twitter_disconnect",
		"twitter_new_message",
		"twitter_send_direct_message",
		"twitter_refresh",
		"twitter_add_friend",
		"view1"
	};

	const gchar   *widgets_disconnected[] = {
		"twitter_connect"
	};

	GList         *list;
	GtkWidget     *w;
	gint           i;

	priv = GET_PRIV (app);

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_connected); i++) {
		w = glade_xml_get_widget (glade, widgets_connected[i]);
		list = g_list_prepend (list, w);
	}

	priv->widgets_connected = list;

	for (i = 0, list = NULL; i < G_N_ELEMENTS (widgets_disconnected); i++) {
		w = glade_xml_get_widget (glade, widgets_disconnected[i]);
		list = g_list_prepend (list, w);
	}

	priv->widgets_disconnected = list;
}



void
twitux_app_state_on_connection (gboolean   connected)
{
	TwituxAppPriv *priv;
	GList         *l;

	priv = GET_PRIV (app);

	for (l = priv->widgets_connected; l; l = l->next) {
		gtk_widget_set_sensitive (l->data, connected);
	}

	for (l = priv->widgets_disconnected; l; l = l->next) {
		gtk_widget_set_sensitive (l->data, !connected);
	}
}

GtkWidget *
twitux_app_get_window (void)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);
	
	return priv->window;
}

void
twitux_app_set_statusbar_msg (gchar *message)
{
	TwituxAppPriv *priv;

	priv = GET_PRIV (app);

	/* Avoid some warnings */
	if (!priv->statusbar || !GTK_IS_STATUSBAR (priv->statusbar))
		return;

	/* conext ID will be always 1 */
	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar), 1);
	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar), 1, message);
}

void
twitux_app_set_friends (GList *friends)
{
	TwituxAppPriv *priv;
	GtkWidget     *menu;
	GtkWidget     *item;
	GList         *list;
	TwituxUser    *user;

	priv = GET_PRIV (app);

	/* Check if we have a menu */
	menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (priv->view_friends));
	if (menu) {
		/* Delete previous items */
		gtk_container_foreach (GTK_CONTAINER (menu),
							   (GtkCallback)gtk_widget_destroy, NULL);
	} else {
		/* New menu */
		menu = gtk_menu_new ();
	}

	if (!friends) {
		/* Temp. menu */
		priv->friends_loaded = FALSE;
		item = gtk_menu_item_new_with_label (_("Fetching friends..."));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		gtk_widget_show (item);
	} else {
		/* Load friends menu from list */
		priv->friends_loaded = TRUE;
		for (list = friends; list; list = list->next) {
			user = (TwituxUser *)list->data;
			twitux_app_add_friend (user);
		}
	}

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (priv->view_friends), menu);
}

void
twitux_app_add_friend (TwituxUser *user)
{
	TwituxAppPriv *priv;
	GtkWidget	  *menu;
	GtkWidget	  *item;

	priv = GET_PRIV (app);

	menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (priv->view_friends));

	if (menu) {
		item = gtk_menu_item_new_with_label (user->name);
		g_signal_connect (item, "activate",
						  G_CALLBACK (app_twitter_view_friend_cb), user);
		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}
}

void
twitux_app_show_notification (gint tweets)
{
	TwituxAppPriv	*priv;
	gchar			*msg;
	gchar			*s;
	gboolean		notify;

	priv = GET_PRIV (app);

	/* Check preferences */
	twitux_conf_get_bool (twitux_conf_get (),
						  TWITUX_PREFS_UI_NOTIFICATION,
						  &notify);

	if (!notify || !gtk_status_icon_is_embedded (priv->status_icon))
		return;

	if (tweets > 1) {
		s = _("tweets");
	} else {
		s = _("tweet");
	}

	msg = g_strdup_printf (_("You have %i new %s."), tweets, s);

	if (priv->notification!= NULL) {
		notify_notification_close (priv->notification, NULL);
	}

	priv->notification =
		notify_notification_new_with_status_icon (PACKAGE_NAME,
												  msg,
												  NULL,
												  priv->status_icon);

	g_object_set(priv->notification, "icon-name", "twitux", NULL) ;

	notify_notification_set_timeout (priv->notification, 3000);
	notify_notification_show (priv->notification, NULL);
	
	g_free (msg);
}

void
twitux_app_set_image (const gchar *file,
                      GtkTreeIter iter)
{
	GtkListStore *store;
	GdkPixbuf	 *pixbuf;
	GError		 *error = NULL;

	pixbuf = gdk_pixbuf_new_from_file (file, &error);

	if (!pixbuf){
		twitux_debug (DEBUG_DOMAIN_SETUP, "Image error: %s: %s",
					  file, error->message);
		g_error_free (error);
		return;
	}
	
	store = twitux_tweet_list_get_store ();

	gtk_list_store_set (store, &iter,
						PIXBUF_AVATAR, pixbuf, -1);
}

void
twitux_app_expand_message (const gchar *name,
                           const gchar *date,
                           const gchar *tweet,
                           GdkPixbuf   *pixbuf)
{
	TwituxAppPriv *priv;
	gchar 		  *title_text;
	
	priv = GET_PRIV (app);

	title_text = g_strdup_printf ("<b>%s</b> - %s", name, date);
		
	twitux_label_set_text (TWITUX_LABEL (priv->expand_label), tweet);
	gtk_label_set_markup (GTK_LABEL (priv->expand_title), title_text);
	g_free (title_text);
	
	if (pixbuf) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (priv->expand_image), pixbuf);
	}
	
	gtk_widget_show (priv->expand_box);
}
