/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2006 Imendio AB
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

#ifndef __TWITUX_PATHS_H__
#define __TWITUX_PATHS_H__

#include <glib.h>

G_BEGIN_DECLS

gchar *twitux_paths_get_xml_path    (const gchar *filename);
gchar *twitux_paths_get_image_path  (const gchar *filename);
gchar *twitux_paths_get_locale_path (void);

G_END_DECLS

#endif /* __TWITUX_PATHS_H__ */
