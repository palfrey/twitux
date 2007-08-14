#if !defined TWITUX_COMMON_H
#define TWITUX_COMMON_H
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 - Alvaro Daniel Morales - <daniel@suruware.com>
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

#include <glib.h>

int tt_is_dir_exist ( const char *dir );

void tt_crear_dir ( const char *dir );

int tt_is_file_exist ( const char *file );

char *tt_get_home_dir ();

char *xml_decode_alloc ( const char* str );

char *tt_md5 ( const char* str, int length );

int tt_unlink ( const char *file );

gchar *tt_get_time ( const char *time );

char *tt_parse_urls_alloc ( const char* message );

char *tt_create_href_alloc ( const char* url );

char* url_encode_alloc (const char* str);

#endif
