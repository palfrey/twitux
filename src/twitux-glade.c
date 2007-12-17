/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2002-2007 Imendio AB
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

#include <config.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libtwitux/twitux-paths.h>

#include "twitux-glade.h"

static void
tagify_bold_labels (GladeXML *xml)
{
	const gchar *str;
	gchar       *s;
	GtkWidget   *label;
	GList       *labels, *l;

	labels = glade_xml_get_widget_prefix (xml, "boldlabel");

	for (l = labels; l; l = l->next) {
		label = l->data;

		if (!GTK_IS_LABEL (label)) {
			g_warning ("Not a label, check your glade file.");
			continue;
		}
 
		str = gtk_label_get_text (GTK_LABEL (label));

		s = g_strdup_printf ("<b>%s</b>", str);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_label_set_label (GTK_LABEL (label), s);
		g_free (s);
	}

	g_list_free (labels);
}

static GladeXML *
get_glade_file (const gchar *filename,
				const gchar *root,
				const gchar *domain,
				const gchar *first_required_widget,
				va_list args)
{
	gchar      *path;
	GladeXML   *gui;
	const char *name;
	GtkWidget **widget_ptr;

	path = twitux_paths_get_glade_path (filename);
	gui = glade_xml_new (path, root, domain);
	g_free (path);

	if (!gui) {
		g_warning ("Couldn't find necessary glade file '%s'", filename);
		return NULL;
	}

	for (name = first_required_widget; name; name = va_arg (args, char *)) {
		widget_ptr = va_arg (args, void *);
		
		*widget_ptr = glade_xml_get_widget (gui, name);
		
		if (!*widget_ptr) {
			g_warning ("Glade file '%s' is missing widget '%s'.",
					   filename, name);
			continue;
		}
	}

	tagify_bold_labels (gui);
	
	return gui;
}

GladeXML *
twitux_glade_get_file (const gchar *filename,
					   const gchar *root,
					   const gchar *domain,
					   const gchar *first_required_widget, ...)
{
	va_list   args;
	GladeXML *gui;

	va_start (args, first_required_widget);

	gui = get_glade_file (filename,
						  root,
						  domain,
						  first_required_widget,
						  args);
	
	va_end (args);

	if (!gui) {
		return NULL;
	}

	return gui;
}

void
twitux_glade_connect (GladeXML *gui,
					  gpointer  user_data,
					  gchar     *first_widget, ...)
{
	va_list      args;
	const gchar *name;
	const gchar *signal;
	GtkWidget   *widget;
	gpointer    *callback;

	va_start (args, first_widget);
	
	for (name = first_widget; name; name = va_arg (args, char *)) {
		signal = va_arg (args, void *);
		callback = va_arg (args, void *);

		widget = glade_xml_get_widget (gui, name);
		if (!widget) {
			g_warning ("Glade file is missing widget '%s', aborting",
					   name);
			continue;
		}

		g_signal_connect (widget,
						  signal,
						  G_CALLBACK (callback),
						  user_data);
	}

	va_end (args);
}
