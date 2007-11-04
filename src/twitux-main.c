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

#include <config.h>
#include <gtk/gtkmain.h>
#include <glib/gi18n.h>
#include <libgnome/gnome-program.h>
#include <libgnomeui/gnome-ui-init.h>

#include <libnotify/notify.h>

#include <libtwitux/twitux-paths.h>

#include "twitux-app.h"
#include "twitux-network.h"

int
main (int argc, char *argv[])
{
	gchar *localedir;
	GnomeProgram *program;

	localedir = twitux_paths_get_locale_path ();
	bindtextdomain (GETTEXT_PACKAGE, localedir);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	g_free (localedir);

	g_set_application_name (_("Twitux"));

	program = gnome_program_init ("twitux", PACKAGE_VERSION,
								  LIBGNOMEUI_MODULE,
								  argc, argv,
								  GNOME_PROGRAM_STANDARD_PROPERTIES,
								  GNOME_PARAM_HUMAN_READABLE_NAME, "Twitux",
								  NULL);

	gtk_window_set_default_icon_name ("twitux");

	/* Start the network */
	twitux_network_new ();

	/* Start libnotify */
	notify_init ("twitux");

	/* Create the ui */
	twitux_app_create ();

	gtk_main ();

	/* Close libnotify */
	notify_uninit ();

	/* Close the network */
	twitux_network_close ();

	/* Clean up the ui */
	g_object_unref (twitux_app_get ());
	g_object_unref (program);

	return 0;
}
