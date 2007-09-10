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

#include "config.h"

#include "twitux-avatar.h"

gboolean
twitux_avatar_pixbuf_is_opaque (GdkPixbuf *pixbuf)
{
	int            width;
	int            height;
	int            rowstride; 
	int            i;
	unsigned char *pixels;
	unsigned char *row;

	if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
		return TRUE;
	}

	width     = gdk_pixbuf_get_width (pixbuf);
	height    = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);

	row = pixels;
	for (i = 3; i < rowstride; i+=4) {
		if (row[i] != 0xff) {
			return FALSE;
		}
	}

	for (i = 1; i < height - 1; i++) {
		row = pixels + (i * rowstride);
		if (row[3] != 0xff || row[rowstride-1] != 0xff) {
			return FALSE;
		}
	}

	row = pixels + ((height-1) * rowstride);
	for (i = 3; i < rowstride; i+=4) {
		if (row[i] != 0xff) {
			return FALSE;
		}
	}

	return TRUE;
}

/* From Pidgin */
void
twitux_avatar_pixbuf_roundify (GdkPixbuf *pixbuf)
{
	int     width;
	int     height;
	int     rowstride;
	guchar *pixels;

	if (!gdk_pixbuf_get_has_alpha (pixbuf)) {
		return;
	}

	width     = gdk_pixbuf_get_width (pixbuf);
	height    = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels    = gdk_pixbuf_get_pixels (pixbuf);

	if (width < 6 || height < 6) {
		return;
	}

	/* Top left */
	pixels[3] = 0;
	pixels[7] = 0x80;
	pixels[11] = 0xC0;
	pixels[rowstride + 3] = 0x80;
	pixels[rowstride * 2 + 3] = 0xC0;

	/* Top right */
	pixels[width * 4 - 1] = 0;
	pixels[width * 4 - 5] = 0x80;
	pixels[width * 4 - 9] = 0xC0;
	pixels[rowstride + (width * 4) - 1] = 0x80;
	pixels[(2 * rowstride) + (width * 4) - 1] = 0xC0;

	/* Bottom left */
	pixels[(height - 1) * rowstride + 3] = 0;
	pixels[(height - 1) * rowstride + 7] = 0x80;
	pixels[(height - 1) * rowstride + 11] = 0xC0;
	pixels[(height - 2) * rowstride + 3] = 0x80;
	pixels[(height - 3) * rowstride + 3] = 0xC0;

	/* Bottom right */
	pixels[height * rowstride - 1] = 0;
	pixels[(height - 1) * rowstride - 1] = 0x80;
	pixels[(height - 2) * rowstride - 1] = 0xC0;
	pixels[height * rowstride - 5] = 0x80;
	pixels[height * rowstride - 9] = 0xC0;
}
