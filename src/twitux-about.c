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

#include <glib/gi18n.h>
#include <gtk/gtkaboutdialog.h>
#include <libgnomevfs/gnome-vfs.h>

#include "twitux-about.h"

#define WEB_SITE "http://twitux.sourceforge.net/"

const gchar *authors[] = {
	"Daniel Morales <daniel@suruware.com>",
	"Brian Pepple <bpepple@fedoraproject.org>",
	NULL
};

const gchar *artists[] = {
	"Architetto Francesco Rollandin",
	NULL
};

const gchar *license[] = {
	N_("Twitux is free software; you can redistribute it and/or modify "
   	   "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 2 of the License, or "
	   "(at your option) any later version."),
	N_("Twitux is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
       "GNU General Public License for more details."),
	N_("You should have received a copy of the GNU General Public License "
   	   "along with Twitux; if not, write to the Free Software Foundation, Inc., "
   	   "59 Temple Place, Suite 330, Boston, MA  02111-1307  USA")
};

static void
about_dialog_activate_link_cb (GtkAboutDialog *about,
							   const gchar    *link,
							   gpointer        data)
{
	GnomeVFSResult result;

	result = gnome_vfs_url_show (link);
	if (result != GNOME_VFS_OK) {
		g_warning ("Couldn't show URL: '%s'", link);
	}
}

void
twitux_about_dialog_new (GtkWindow *parent)
{
	gchar *license_trans;

	gtk_about_dialog_set_url_hook (about_dialog_activate_link_cb, NULL, NULL);

	license_trans = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n",
								 _(license[2]), "\n\n", NULL);

	/* TODO: Add the logo once we get a new icon */
	gtk_show_about_dialog (parent,
						   "authors", authors,
						   "artists", artists,
						   "comments", _("A GNOME client for Twitter."),
						   "copyright", _("Copyright \xc2\xa9 2007 Daniel Morales"),
						   "license", license_trans,
						   "translator-credits", _("translator-credits"),
						   "version", PACKAGE_VERSION,
						   "website", WEB_SITE,
						   "wrap-license", TRUE,
						   "logo-icon-name", "twitux",
						   NULL);

	g_free (license_trans);
}
