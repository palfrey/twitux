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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _XOPEN_SOURCE
#include <time.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include <openssl/md5.h>

#include "main.h"
#include "common.h"


#define ACCEPT_LETTER_URL "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;/?@&=+$,-_.!~*'%"

#define ACCEPT_LETTER_USER "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"

time_t strtotime ( const char *s_time );


int tt_is_dir_exist ( const char *dir )
{	
	return g_file_test ( dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR );
}


int tt_is_file_exist ( const char *file )
{
	return g_file_test ( file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR );
}


int tt_unlink ( const char *file )
{
	if ( !tt_is_file_exist ( file ) ) return 1;
		
	return remove ( file );
}


void tt_crear_dir (const char *dir)
{
	mkdir ( dir, S_IRWXU );
}

char* tt_get_home_dir ()
{
	return g_build_path ( G_DIR_SEPARATOR_S, getenv( "HOME" ), ".twitux", NULL );
}


char* xml_decode_alloc (const char* str) {

	char* buf = NULL;
	unsigned char* pbuf = NULL;
	int len = 0;

	if (!str) return NULL;
	
	len = strlen (str)*3;

	buf = malloc (len+1);

	memset (buf, 0, len+1);

	pbuf = (unsigned char*)buf;
	
	while(*str) {

		if ( !memcmp ( str, "&amp;lt;", 8 ) ) {
			strcat ( (char*)pbuf++, "&" );
			strcat ( (char*)pbuf++, "l" );
			strcat ( (char*)pbuf++, "t" );
			strcat ( (char*)pbuf++, ";" );
			str += 8;
		} else if ( !memcmp ( str, "&amp;gt;", 8 ) ) {
			strcat ( (char*)pbuf++, "&" );
			strcat ( (char*)pbuf++, "g" );
			strcat ( (char*)pbuf++, "t" );
			strcat ( (char*)pbuf++, ";" );
			str += 8;
		} else if ( !memcmp ( str, "\n", 1 ) ) {
			strcat ( (char*)pbuf++, "." );
			strcat ( (char*)pbuf++, " " );
			str += 1;
		} else if ( !memcmp ( str, "&", 1 ) ) {
			strcat ( (char*)pbuf++, "&" );
			strcat ( (char*)pbuf++, "a" );
			strcat ( (char*)pbuf++, "m" );
			strcat ( (char*)pbuf++, "p" );
			strcat ( (char*)pbuf++, ";" );
			str += 1;
		}  else {
			*pbuf++ = *str++;
		}
	}
	return buf;

}


char* url_encode_alloc (const char* str) {
	static const int force_encode_all = TRUE;
	const char* hex = "0123456789abcdef";

	char* buf = NULL;
	unsigned char* pbuf = NULL;
	int len = 0;

	if (!str) return NULL;

	len = strlen(str)*3;
	buf = malloc(len+1);
	memset(buf, 0, len+1);
	pbuf = (unsigned char*)buf;

	while(*str) {
		unsigned char c = (unsigned char)*str;
		if (c == ' ')
			*pbuf++ = '+';
		else if (c & 0x80 || force_encode_all) {
			*pbuf++ = '%';
			*pbuf++ = hex[c >> 4];
			*pbuf++ = hex[c & 0x0f];
		} else
			*pbuf++ = c;
		str++;
	}

	return buf;

}

char* tt_md5 ( const char* str, int length )
{
	unsigned char digest[MD5_DIGEST_LENGTH];
	char *hexChars = "0123456789abcdef";
	char* buf = NULL;
	int len = (MD5_DIGEST_LENGTH*2);
	int i;
	
	if (!str) return NULL;
		
	buf = malloc ( len +1);
	memset ( buf, 0, len+1 );
	
	MD5 ( (unsigned char *)str, length, digest );
	
	// convertir a hexadecimal
	for ( i=0; i<MD5_DIGEST_LENGTH; i++ ){

		buf[i*2]=hexChars[(digest[i]>>4)&0xF];

		buf[(i*2)+1]=hexChars[digest[i]&0xF];

	}

	buf[len]='\0';

	return buf;

}


gchar * tt_get_time ( const char *stime )
{
	struct tm *tm_gmt;

	time_t tm_actual = time(NULL);
	time_t ts_gmt;
	time_t ts_post;
	int diff;

	// Obtengo el timestamp del post
	ts_post = strtotime ( stime );

	// Obtengo el timestamp actual en GMT
	tm_gmt = gmtime ( &tm_actual );
	ts_gmt = mktime ( tm_gmt );

	diff = (int)(ts_gmt - ts_post);
	
	if ( diff < 0 ) {// Atrasos de horas respecto al servo
		return g_strdup ( _("1 second ago") );
	}
	
	if ( diff == 60 ) {

		return g_strdup ( _("1 minute ago") );

	} else if (diff < 60 ) {

		return g_strdup_printf ( _("%i seconds ago"), diff );

	} else {

		diff = diff/60;//paso a minutos

		if ( diff == 60 ) {

			return g_strdup ( _("1 hour ago") );

		} else if ( diff < 60 ) {

			return g_strdup_printf ( _("%i minutes ago"), diff );

		} else {

			diff = diff/60;//paso a horas

			if ( diff == 24 ) {

				return g_strdup ( _("1 day ago") );

			} else if ( diff < 24 ) {

				return g_strdup_printf ( _("%i hours ago"), diff );
	
			} else {

				diff = diff/24;//paso a dias
				
				if ( diff == 30 ) {

					return g_strdup ( _("1 month ago") );

				} else if ( diff < 30 ) {

					return g_strdup_printf ( _("%i days ago"), diff );

				} else {

					return g_strdup_printf ( _("%i months ago"), (diff/30) );

				}

			}

		}

	}

	return 0;

}


time_t strtotime(const char *s_time)
{
	struct tm tiempo;

	if(!s_time ) return (time_t)0;

	strptime (s_time, "%a %b %d %T +0000 %Y", &tiempo);

	return mktime(&tiempo);

}


char* tt_create_href_alloc ( const char* url )
{
	return g_strdup_printf ( "<a href='%s'>%s</a>", url, url );
}


char* tt_parse_urls_alloc ( const char* message )
{
	const char* ptr = message;
	const char* last = ptr;
	char* ret = NULL;
	int len = 0;

	while(*ptr) {
		
		gboolean nicktl = !(strncmp(ptr, "@", 1));
		
		if (!strncmp(ptr, "http://", 7) || !strncmp(ptr, "ftp://", 6) || nicktl ) {
	
			char* link;
			char* tiny_url;
			const char* tmp;

			if ( nicktl ) {
				ptr++;
			}

			if (last != ptr) {
				len += (ptr-last);
				if (!ret) {
					ret = malloc(len+1);
					memset(ret, 0, len+1);
				} else ret = realloc(ret, len+1);
				strncat(ret, last, ptr-last);
			}

			tmp = ptr;

			if ( nicktl ) { // Parseando nick

				while(*tmp && strchr(ACCEPT_LETTER_USER, *tmp)) tmp++;

			} else { // Parseando URL

				while(*tmp && strchr(ACCEPT_LETTER_URL, *tmp)) tmp++;

			}

			link = malloc(tmp-ptr+1);
			memset(link, 0, tmp-ptr+1);
			memcpy(link, ptr, tmp-ptr);

			tiny_url = tt_create_href_alloc (link);

			if (tiny_url) {
				free(link);
				link = tiny_url;
			}

			len += strlen(link);

			if (!ret) {
				ret = malloc(len+1);
				memset(ret, 0, len+1);
			} else ret = realloc(ret, len+1);

			strcat(ret, link);
			free(link);
			ptr = last = tmp;

		} else {

			ptr++;

		}

	}
	if (last != ptr) {

		len += (ptr-last);

		if (!ret) {

			ret = malloc(len+1);
			memset(ret, 0, len+1);

		} else ret = realloc(ret, len+1);
		strncat(ret, last, ptr-last);
	}

	return ret;

}
