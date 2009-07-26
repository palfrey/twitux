/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007-2009 Daniel Morales <daniminas@gmail.com>
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
 * Authors: Daniel Morales <daniminas@gmail.com>
 *
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <stdlib.h>

#include <libtwitux/twitux-debug.h>
#include <libtwitux/twitux-conf.h>
#ifdef HAVE_GNOME_KEYRING
#include <libtwitux/twitux-keyring.h>
#endif

#include "twitux.h"
#include "twitux-network.h"
#include "twitux-parser.h"
#include "twitux-app.h"
#include "twitux-send-message-dialog.h"
#include "twitux-lists-dialog.h"
#include "twitux-tweet-list.h"

#define DEBUG_DOMAIN	  "Network"
#define TWITUX_HEADER_URL "https://twitux.sourceforge.net/client.xml"

typedef struct {
	gchar        *src;
	GtkTreePath   *path;
} TwituxImage;

static void network_get_data		(const gchar           *url,
									 SoupSessionCallback    callback,
									 gpointer               data);
static void network_post_data		(const gchar           *url,
									 gchar                 *formdata,
									 SoupSessionCallback    callback,
									 gpointer               data);
static gboolean	network_check_http 	(gint                   status_code);
static void network_parser_free_lists (void);

/* libsoup callbacks */
static void network_cb_on_login		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_post		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_message	(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_timeline	(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_users	    (SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_image		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_add		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_del		(SoupSession           *session,
									 SoupMessage           *msg,
									 gpointer               user_data);
static void network_cb_on_auth		(SoupSession           *session,
									 SoupMessage           *msg,
									 SoupAuth              *auth,
									 gboolean               retrying,
									 gpointer               data);

/* Autoreload timeout functions */
static gboolean 	network_timeout			(gpointer user_data);
static void			network_timeout_new		(void);

void
network_parse_limit_headers(SoupMessageHeaders *hdrs);

static SoupSession			*soup_connection = NULL;
static GList				*user_friends = NULL;
static GList				*user_followers = NULL;
static gboolean				 processing = FALSE;
static gchar				*current_timeline = NULL;
static guint				 timeout_id;
gchar                *global_username = NULL;
static gchar                *global_password = NULL;
static gint				hourly_limit = 100; // standard twitter limit, assume by default
static time_t			reset_time = 0; // next reset time
static gboolean			logged_in = FALSE;

/* This function must be called at startup */
void
twitux_network_new (void)
{
	TwituxConf	*conf;
	gboolean	check_proxy = FALSE;
 
	/* Close previous networking */
	if (soup_connection) {
		twitux_network_close ();
	}

	/* Async connections */
	soup_connection = soup_session_async_new_with_options (SOUP_SESSION_MAX_CONNS,
														   8,
														   NULL);
	twitux_debug (DEBUG_DOMAIN, "Libsoup (re)started");

	/* Set the proxy, if configuration is set */
	conf = twitux_conf_get ();
	twitux_conf_get_bool (conf,
						  TWITUX_PROXY_USE,
						  &check_proxy);

	if (check_proxy) {
		gchar *server, *proxy_uri;
		gint port;

		/* Get proxy */
		twitux_conf_get_string (conf,
								TWITUX_PROXY_HOST,
								&server);
		twitux_conf_get_int (conf,
							 TWITUX_PROXY_PORT,
							 &port);

		if (server && server[0]) {
			SoupURI *suri;

			check_proxy = FALSE;
			twitux_conf_get_bool (conf,
								  TWITUX_PROXY_USE_AUTH,
								  &check_proxy);

			/* Get proxy auth data */
			if (check_proxy) {
				char *user, *password;

				twitux_conf_get_string (conf,
										TWITUX_PROXY_USER,
										&user);
				twitux_conf_get_string (conf,
										TWITUX_PROXY_PASS,
										&password);

				proxy_uri = g_strdup_printf ("http://%s:%s@%s:%d",
											 user,
											 password,
											 server,
											 port);

				g_free (user);
				g_free (password);
			} else {
				proxy_uri = g_strdup_printf ("http://%s:%d", 
											 server, port);
			}

			twitux_debug (DEBUG_DOMAIN, "Proxy uri: %s",
						  proxy_uri);

			/* Setup proxy info */
			suri = soup_uri_new (proxy_uri);
			g_object_set (G_OBJECT (soup_connection),
						  SOUP_SESSION_PROXY_URI,
						  suri,
						  NULL);

			soup_uri_free (suri);
			g_free (server);
			g_free (proxy_uri);
		}
	}
}


/* Cancels requests, and unref libsoup. */
void
twitux_network_close (void)
{
	/* Close all connections */
	twitux_network_stop ();

	network_parser_free_lists ();

	g_object_unref (soup_connection);

	if (current_timeline) {
		g_free (current_timeline);
		current_timeline = NULL;
	}

	twitux_debug (DEBUG_DOMAIN, "Libsoup closed");
}


/* Cancels all pending requests in session. */
void
twitux_network_stop	(void)
{
	twitux_debug (DEBUG_DOMAIN,"Cancelled all connections");

	soup_session_abort (soup_connection);
}


/* Login in Twitter */
void
twitux_network_login (const char *username, const char *password)
{
	if (logged_in)
		return;
	twitux_debug (DEBUG_DOMAIN, "Begin login.. ");

	g_free (global_username);
	global_username = g_strdup (username);
	g_free (global_password);
	global_password = g_strdup (password);

	twitux_app_set_statusbar_msg (_("Connecting..."));

	/* HTTP Basic Authentication */
	g_signal_connect (soup_connection,
					  "authenticate",
					  G_CALLBACK (network_cb_on_auth),
					  NULL);

	/* Verify credentials */
	network_get_data (TWITUX_API_LOGIN, network_cb_on_login, NULL);
}


/* Logout current user */
void twitux_network_logout (void)
{
	twitux_network_new ();
	
	twitux_debug (DEBUG_DOMAIN, "Logout");
	
	logged_in = FALSE;
}


/* Post a new tweet - text must be Url encoded */
void
twitux_network_post_status (const gchar *text, gint64 reply_id)
{
	gchar *formdata;

	if (reply_id == -1)
		formdata = g_strdup_printf ("source=twitux&status=%s", text);
	else
		formdata = g_strdup_printf ("source=twitux&status=%s&in_reply_to_status_id=%lld", text, reply_id);
	twitux_debug (DEBUG_DOMAIN,
					 "Tweeting %s",text);

	network_post_data (TWITUX_API_POST_STATUS,
					   formdata,
					   network_cb_on_post,
					   NULL);
}


/* Send a direct message to a follower - text must be Url encoded  */
void
twitux_network_send_message (const gchar *friend,
							 const gchar *text)
{
	gchar *formdata;

	formdata = g_strdup_printf ( "user=%s&text=%s", friend, text);
	
	network_post_data (TWITUX_API_SEND_MESSAGE,
					   formdata,
					   network_cb_on_message,
					   NULL);
}

void
twitux_network_refresh (void)
{
	if (!current_timeline || processing)
	{
		twitux_debug (DEBUG_DOMAIN, "Can't refresh (ct %p, processing %d)!", current_timeline, processing);
		return;
	}

	/* UI */
	twitux_app_set_statusbar_msg (_("Loading timeline..."));

	processing = TRUE;
	network_get_data (current_timeline, network_cb_on_timeline, NULL);
}

/* Get and parse a timeline */
void
twitux_network_get_timeline (const gchar *url_timeline)
{
	if (processing)
		return;

	parser_reset_lastid ();

	/* UI */
	twitux_app_set_statusbar_msg (_("Loading timeline..."));

	processing = TRUE;
	network_get_data (url_timeline, network_cb_on_timeline, g_strdup(url_timeline));
}

/* Get a user timeline */
void
twitux_network_get_user (const gchar *username)
{
	gchar *user_timeline;
	gchar *user_id;

	if (!username){
		twitux_conf_get_string (twitux_conf_get (),
								TWITUX_PREFS_AUTH_USER_ID,
								&user_id);
	} else {
		user_id = g_strdup (username);
	}

	if(!G_STR_EMPTY (user_id)) {
		user_timeline = g_strdup_printf (TWITUX_API_TIMELINE_USER,
										 user_id);
	
		twitux_network_get_timeline (user_timeline);
		g_free (user_timeline);
	}

	if (user_id)
		g_free (user_id);
}

/* Get authenticating user's friends(following)
 * Returns:
 * 		NULL: Friends will be fetched
 * 		GList: The list of friends (fetched previously)
 */
GList *
twitux_network_get_friends (void)
{
	gboolean friends = TRUE;

	if (user_friends)
		return user_friends;
	
	network_get_data (TWITUX_API_FOLLOWING,
					  network_cb_on_users,
					  GINT_TO_POINTER(friends));
	
	return NULL;
}


/* Get the authenticating user's followers
 * Returns:
 * 		NULL: Followers will be fetched
 * 		GList: The list of friends (fetched previously)
 */
GList *
twitux_network_get_followers (void)
{
	gboolean friends = FALSE;

	if (user_followers)
		return user_followers;

	network_get_data (TWITUX_API_FOLLOWERS,
					  network_cb_on_users,
					  GINT_TO_POINTER(friends));

	return NULL;
}


/* Get an image from servers */
void
twitux_network_get_image (const gchar  *url_image,
						  GtkTreeIter   iter)
{
	gchar	*image_file;
	gchar   *image_name;
	GtkListStore *store;

	TwituxImage *image;

	/* save using the filename */
	image_name = strrchr (url_image, '/');
	if (image_name && image_name[1] != '\0') {
		image_name++;
	} else {
		image_name = "twitux_unknown_image";
	}

	image_file = g_build_filename (g_get_home_dir(), ".gnome2",
								   TWITUX_CACHE_IMAGES,
								   image_name, NULL);

	/* check if image already exists */
	if (g_file_test (image_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {		
		/* Set image from file here */
		twitux_app_set_image (image_file, iter);
		g_free (image_file);
		return;
	}

	store = twitux_tweet_list_get_store ();
	image = g_new0 (TwituxImage, 1);
	image->src  = g_strdup (image_file);
	image->path = gtk_tree_model_get_path(GTK_TREE_MODEL (store), &iter);

	g_free (image_file);

	/* Note: 'image' will be freed in 'network_cb_on_image' */
	network_get_data (url_image, network_cb_on_image, image);
}



/* Add a user to follow */
void
twitux_network_add_user (const gchar *username)
{
	gchar *url;
	
	if (G_STR_EMPTY (username))
		return;
	
	url = g_strdup_printf (TWITUX_API_FOLLOWING_ADD, username);

	network_post_data (url, NULL, network_cb_on_add, NULL);

	g_free (url);
}


/* Add a user to follow */
void
twitux_network_del_user (TwituxUser *user)
{
	gchar *url;
	
	if (!user || !user->screen_name)
		return;
	
	url = g_strdup_printf (TWITUX_API_FOLLOWING_DEL, user->screen_name);

	network_post_data (url, NULL, network_cb_on_del, user);

	g_free (url);
}

typedef struct
{
	SoupSessionCallback callback;
	SoupMessage *msg;
	gpointer data;
	guint event;
} GetData;

static gboolean
_network_timeout(void *data)
{
	GetData *gd = (GetData*)data;
	twitux_debug (DEBUG_DOMAIN, "Get failure!");
	soup_session_cancel_message(soup_connection, gd->msg, SOUP_STATUS_CANCELLED);
	return FALSE;
}

static void
_network_ok (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	GetData *gd = (GetData*)user_data;
	g_source_remove (gd->event);
	gd->callback (session, msg, gd->data);
	g_free(gd);
}

/* Get data from net */
static void
network_get_data (const gchar           *url,
				  SoupSessionCallback    callback,
				  gpointer               data)
{
	SoupMessage *msg;
	GetData *gd;

	twitux_debug (DEBUG_DOMAIN, "Get: %s",url);

	msg = soup_message_new ( "GET", url );
	gd = g_new(GetData, 1);
	gd->callback = callback;
	gd->data = data;
	gd->event = g_timeout_add_seconds(15, _network_timeout, gd);
	gd->msg = msg;

	soup_session_queue_message (soup_connection, msg, _network_ok, gd);
}


/* Private: Post data to net */
static void
network_post_data (const gchar           *url,
				   gchar                 *formdata,
				   SoupSessionCallback    callback,
				   gpointer               data)
{
	SoupMessage *msg;

	twitux_debug (DEBUG_DOMAIN, "Post: %s",url);

	msg = soup_message_new ("POST", url);
	
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client", PACKAGE_NAME);
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client-Version", PACKAGE_VERSION);
	soup_message_headers_append (msg->request_headers,
								 "X-Twitter-Client-URL", TWITUX_HEADER_URL);

	if (formdata)
	{
		soup_message_set_request (msg, 
								  "application/x-www-form-urlencoded",
								  SOUP_MEMORY_TAKE,
								  formdata,
								  strlen (formdata));
	}

	soup_session_queue_message (soup_connection, msg, callback, data);
}


/* Check HTTP response code */
static gboolean
network_check_http (gint status_code)
{
	if (status_code == 401) {
		twitux_app_set_statusbar_msg (_("Access denied."));

	} else if (SOUP_STATUS_IS_CLIENT_ERROR (status_code)) {
		twitux_app_set_statusbar_msg (_("HTTP communication error."));

	} else if(SOUP_STATUS_IS_SERVER_ERROR (status_code)) {
		twitux_app_set_statusbar_msg (_("Internal server error."));

	} else if (!SOUP_STATUS_IS_SUCCESSFUL (status_code)) {
		twitux_app_set_statusbar_msg (_("Stopped"));

	} else if (SOUP_STATUS_IS_TRANSPORT_ERROR(status_code)) {
		twitux_app_set_statusbar_msg(_("Transport error."));
		twitux_debug (DEBUG_DOMAIN,
					  "Transport error: %i",status_code);

	} else {
		return TRUE;
	}
	
	return FALSE;
}

/* Callback to free every element on a User list */
static void
network_free_user_list_each (gpointer user,
							 gpointer data)
{
	TwituxUser *usr;

	if (!user)
		return;

	usr = (TwituxUser *)user;
	parser_free_user (user);
}


/* Free a list of Users */
static void
network_parser_free_lists ()
{
	if (user_friends) {
		g_list_foreach (user_friends, network_free_user_list_each, NULL);
		g_list_free (user_friends);
		user_friends = NULL;
		twitux_debug (DEBUG_DOMAIN,
					  "Friends freed");
	}

	if (user_followers) {
		g_list_foreach (user_followers, network_free_user_list_each, NULL);
		g_list_free (user_followers);
		user_followers = NULL;
		twitux_debug (DEBUG_DOMAIN,
					  "Followers freed");
	}
}



/* HTTP Basic Authentication */
static void
network_cb_on_auth (SoupSession  *session,
					SoupMessage  *msg,
					SoupAuth     *auth,
					gboolean      retrying,
					gpointer      data)
{
	/* Don't bother to continue if there is no user_id */
	if (G_STR_EMPTY (global_username)) {
		return;
	}

	/* verify that the password has been set */
	if (!G_STR_EMPTY (global_password))
		soup_auth_authenticate (auth, global_username, 
								global_password);
}


/* On verify credentials */
static void
network_cb_on_login (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	twitux_debug (DEBUG_DOMAIN,
				  "Login response: %i",msg->status_code);

	if (network_check_http (msg->status_code)) {
		logged_in = TRUE;
		twitux_app_state_on_connection (TRUE);
		return;
	}

	twitux_app_state_on_connection (FALSE);
}


/* On post a tweet */
static void
network_cb_on_post (SoupSession *session,
					SoupMessage *msg,
					gpointer     user_data)
{
	if (network_check_http (msg->status_code)) {
		twitux_app_set_statusbar_msg (_("Status Sent"));
	}
	else
		twitux_debug (DEBUG_DOMAIN,
					 "Tweet response reason: %s",msg->reason_phrase);
	
	twitux_debug (DEBUG_DOMAIN,
				  "Tweet response: %i",msg->status_code);
}


/* On send a direct message */
static void
network_cb_on_message (SoupSession *session,
					   SoupMessage *msg,
					   gpointer     user_data)
{
	if (network_check_http (msg->status_code)) {
		twitux_app_set_statusbar_msg (_("Message Sent"));
	}

	twitux_debug (DEBUG_DOMAIN,
				  "Message response: %i",msg->status_code);
}

void
network_parse_limit_headers(SoupMessageHeaders *hdrs)
{
	time_t seconds, now, reset;
	double speed;
	const char *temp;
	gint limit;

	temp = soup_message_headers_get(hdrs, "X-RateLimit-Reset");
	if (temp == NULL)
		return;
	reset = atoi(temp);
	temp = soup_message_headers_get(hdrs, "X-RateLimit-Remaining");
	if (temp == NULL)
		return;
	limit = atoi(temp);
	twitux_debug(DEBUG_DOMAIN, "Reset: %lu, Remaining %d", reset, limit);

	/* seconds until the next reset */
	now = time(NULL);
	if (reset>now)
		seconds = reset-now;
	else /* assume really close, so 1 second */
		seconds = 1;
	/* max requests per second given current limit and time to next reset */
	speed = limit/(seconds*1.0);

	/* requests per hour */
	hourly_limit = speed*60*60;
	if (hourly_limit > 1000) /* crazy numbers */
		hourly_limit = 1000;

	twitux_debug (DEBUG_DOMAIN, 
			"Remaining limit is %d, speed is %f, seconds is %lu, hourly limit is %f",limit,speed,seconds,speed*60*60);
	reset_time = reset;
}

/* On get a timeline */
static void
network_cb_on_timeline (SoupSession *session,
						SoupMessage *msg,
						gpointer     user_data)
{
	gchar        *new_timeline = NULL;
	
	if (user_data){
		new_timeline = (gchar *)user_data;
	}

	twitux_debug (DEBUG_DOMAIN,
				  "Timeline response: %i",msg->status_code);

	processing = FALSE;

	/* Timeout */
	network_timeout_new ();

	/* Check response */
	if (!network_check_http (msg->status_code)) {
		if (new_timeline)
			g_free (new_timeline);
		if (msg->status_code == 400) // check anyway
			network_parse_limit_headers(msg->response_headers);
		return;
	}

	network_parse_limit_headers(msg->response_headers);

	twitux_debug (DEBUG_DOMAIN, "Parsing timeline");

	/* Parse and set ListStore */
	if (twitux_parser_timeline (msg->response_body->data, msg->response_body->length)) {
		twitux_app_set_statusbar_msg (_("Timeline Loaded"));
	} else {
		twitux_app_set_statusbar_msg (_("Timeline Parser Error."));
	}
	if (new_timeline){
		if (current_timeline)
			g_free (current_timeline);
		current_timeline = new_timeline;
	}
}


/* On get user followers */
static void
network_cb_on_users (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	gboolean  friends = GPOINTER_TO_INT(user_data);
	GList    *users;
		
	twitux_debug (DEBUG_DOMAIN,
				  "Users response: %i",msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse user list */
	twitux_debug (DEBUG_DOMAIN, "Parsing user list");

	users = twitux_parser_users_list (msg->response_body->data,
									  msg->response_body->length);

	/* check if it ok, and if it is a followers or following list */
	if (users && friends){
		/* Friends retrived */
		user_friends = users;
		twitux_lists_dialog_load_lists (user_friends);
	} else if (users){
		/* Followers list retrived */
		user_followers = users;
		twitux_message_set_followers (user_followers);
	} else {
		twitux_app_set_statusbar_msg (_("Users parser error."));
	}
}


/* On get a image */
static void
network_cb_on_image (SoupSession *session,
					 SoupMessage *msg,
					 gpointer     user_data)
{
	TwituxImage *image = (TwituxImage *)user_data;

	twitux_debug (DEBUG_DOMAIN,
				  "Image response: %i", msg->status_code);

	/* check response */
	if (network_check_http (msg->status_code)) {
		/* Save image data */
		if (g_file_set_contents (image->src,
								 msg->response_body->data,
								 msg->response_body->length,
								 NULL)) {
			/* Set image from file here (image_file) */
			GtkListStore *store;
			GtkTreeIter iter;

			store = twitux_tweet_list_get_store ();
			gtk_tree_model_get_iter(GTK_TREE_MODEL (store), &iter, image->path); 
			twitux_app_set_image (image->src, iter);
		}
	}

	gtk_tree_path_free (image->path);
	g_free (image->src);
	g_free (image);
}


/* On add a user */
static void
network_cb_on_add (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	TwituxUser *user;

	twitux_debug (DEBUG_DOMAIN,
				  "Add user response: %i", msg->status_code);
	
	/* Check response */
	if (!network_check_http (msg->status_code))
		return;

	/* parse new user */
	twitux_debug (DEBUG_DOMAIN, "Parsing new user");

	user = twitux_parser_single_user (msg->response_body->data,
									  msg->response_body->length);

	if (user) {
		user_friends = g_list_append ( user_friends, user );
		twitux_app_set_statusbar_msg (_("Friend Added"));
	} else {
		twitux_app_set_statusbar_msg (_("Failed to add user"));
	}
}

/* On remove a user */
static void
network_cb_on_del (SoupSession *session,
				   SoupMessage *msg,
				   gpointer     user_data)
{
	twitux_debug (DEBUG_DOMAIN,
				  "Delete user response: %i", msg->status_code);

	if (network_check_http (msg->status_code)) {		
		twitux_app_set_statusbar_msg (_("Friend Removed"));
	} else {
		twitux_app_set_statusbar_msg (_("Failed to remove user"));
	}
	
	if (user_data) {
		TwituxUser *user = (TwituxUser *)user_data;
		user_friends = g_list_remove (user_friends, user);
		parser_free_user (user);
	}
}

static void
network_timeout_new (void)
{
	gint minutes;
	guint reload_time;

	if (timeout_id) {
		twitux_debug (DEBUG_DOMAIN,
					  "Stopping timeout id: %i", timeout_id);

		g_source_remove (timeout_id);
	}

	twitux_conf_get_int (twitux_conf_get (),
						 TWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
						 &minutes);
	if (minutes <=0) /* should only be -1, but handle the zero case as well */
	{
		gint limit;
		limit = hourly_limit;

		if (limit == -1)
			limit = 5; /* pick 'small' number if limit is -1 as this indicates we're updating too fast */
		else
		{
			/* Limit grabbing to 2/3rds of max possible rate */
			limit *=2;
			limit /=3;
		}

		if (limit == 0 && reset_time != 0) // if we've run out, but we know when reset is...
		{
			time_t now = time(NULL);
			if (reset_time>now)
				reload_time = reset_time-now+3; // add a couple of extra seconds to allow for offset issues between here and twitter
			else
				reload_time = 5; // assume right reset time is shortly in the future;
			reload_time *=1000; // convert to ms
			twitux_debug(DEBUG_DOMAIN, "Ran out of limit, so waiting for reset");
		}
		else
			reload_time = (60*60*1000)/limit;
		if (reload_time < 20000) /* less than 20 seconds. Probably still too low, but traps some of the crazier cases */
			reload_time = 20000;
		twitux_debug (DEBUG_DOMAIN,
				"Limit is %d, reload time is %d",limit,reload_time);
	}
	else
		reload_time = minutes * 60 * 1000;

	timeout_id = g_timeout_add (reload_time,
								network_timeout,
								NULL);

	twitux_debug (DEBUG_DOMAIN,
				  "Starting timeout id: %i", timeout_id);
}

static gboolean
network_timeout (gpointer user_data)
{
	if (!current_timeline || processing)
		return FALSE;

	/* UI */
	twitux_app_set_statusbar_msg (_("Reloading timeline..."));

	twitux_debug (DEBUG_DOMAIN,
				  "Auto reloading. Timeout: %i", timeout_id);

	processing = TRUE;
	network_get_data (current_timeline, network_cb_on_timeline, NULL);

	timeout_id = 0;

	return FALSE;
}
