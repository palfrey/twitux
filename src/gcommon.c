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

#include <config.h>

#include <gtk/gtk.h>

#include "main.h"
#include "twitter.h"
#include "gcommon.h"


GdkPixbuf *cargar_pixbuf ( const char *path )
{
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	pixbuf = gdk_pixbuf_new_from_file ( path, &error );

	if ( !pixbuf ){

		g_warning ( "Loading image: %s: %s\n", path, error->message );

		g_error_free ( error );

		return NULL;
	}

	return pixbuf;

}


GtkWidget *crear_menu ( const char *texto, GtkWidget *parent )
{
	GtkWidget *menu;

	menu = gtk_menu_item_new_with_mnemonic ( texto );
	gtk_widget_show ( menu );

	gtk_container_add ( GTK_CONTAINER ( parent ), menu );

	return menu;

}


GtkWidget *crear_stock_mi ( const char *stock, GtkWidget *parent, GtkAccelGroup *accel_group )
{
	GtkWidget *nmi;

	nmi = gtk_image_menu_item_new_from_stock ( stock, accel_group );
	gtk_widget_show ( nmi );

	gtk_container_add ( GTK_CONTAINER ( parent ), nmi );

	return nmi;

}


GtkWidget *crear_menu_separador ( GtkWidget *parent )
{
	GtkWidget *separador;

	separador = gtk_separator_menu_item_new ();
	gtk_widget_show ( separador );

	gtk_container_add ( GTK_CONTAINER ( parent ), separador );
	gtk_widget_set_sensitive ( separador, FALSE );

	return separador;

}


GtkWidget *crear_menu_check ( GtkWidget *parent, const char *txt, gboolean active )
{
	GtkWidget *check;

	check = gtk_check_menu_item_new_with_mnemonic ( txt );
	gtk_widget_show ( check );

	gtk_container_add ( GTK_CONTAINER (parent), check );
	gtk_check_menu_item_set_active ( GTK_CHECK_MENU_ITEM ( check ), active );

	return check;

}


GtkWidget *crear_menu_radio ( GSList *group, const char *txt, gboolean active, GtkWidget *parent )
{
	GtkWidget *item;

	item = gtk_radio_menu_item_new_with_label ( group, txt );
	gtk_widget_show ( item );

	gtk_container_add ( GTK_CONTAINER ( parent ), item );
	gtk_check_menu_item_set_active ( GTK_CHECK_MENU_ITEM ( item ), active );

	return item;

}


void tt_ver_ocultar_widget ( GtkWidget *widget, gboolean ver )
{
	if ( ver ){
		gtk_widget_show ( widget );
	} else {
		gtk_widget_hide ( widget );
	}
}


void tt_cambiar_mensaje_estado ( TwiTux *twitter, const char *mensaje )
{
	GtkWidget *statusbar = twitter->principal->barra_estado;
	gchar *tmpstring;

	// Mensaje en la barra de estado
	if ( statusbar && GTK_IS_STATUSBAR ( statusbar ) ) {
	
		gtk_statusbar_pop ( GTK_STATUSBAR ( statusbar ), twitter->principal->barra_estado_cid );

		gtk_statusbar_push ( GTK_STATUSBAR ( statusbar ) , twitter->principal->barra_estado_cid , mensaje );
	}

	// Tooltip del icono en area de notificacion
	if ( twitter->notify_icon && GTK_IS_STATUS_ICON ( twitter->notify_icon ) ) {

		tmpstring = g_strdup_printf ( "%s - %s", TWITTER_HEADER_CLIENT, mensaje );

		gtk_status_icon_set_tooltip ( twitter->notify_icon, tmpstring );
	
		g_free ( tmpstring );
	}

}


void tt_clear_list ( GtkWidget *lista_tree )
{
	GtkTreeModel *model;

	// Borro los estados anteriores
	model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( lista_tree ) );
	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

}


void tt_enable_disable_widgets ( gboolean flag, TwiTuxWindow *window )
{
	if ( flag ){
		gtk_widget_set_sensitive ( window->menuitem_conectar, FALSE );
		gtk_widget_set_sensitive ( window->menuitem_desconectar, TRUE );
	} else {
		gtk_widget_set_sensitive ( window->menuitem_conectar, TRUE );
		gtk_widget_set_sensitive ( window->menuitem_desconectar, FALSE );
	}

	gtk_widget_set_sensitive ( window->menuitem_recargar, flag );
	gtk_widget_set_sensitive ( window->menuitem_enviar_estado, flag );
	gtk_widget_set_sensitive ( window->menuitem_amigos_timeline, flag );
	gtk_widget_set_sensitive ( window->menuitem_amigos, flag );
	gtk_widget_set_sensitive ( window->menuitem_mensajes_directos, flag );
	gtk_widget_set_sensitive ( window->menuitem_respuestas, flag );
	gtk_widget_set_sensitive ( window->menuitem_mi_timeline, flag );
	gtk_widget_set_sensitive ( window->menuitem_enviar_mensaje, flag );

}


static void friend_list_remove ( gpointer data, gpointer userdata )
{
	TwiTuxUser *user;

	if ( !data )
		return;

	user = TWITUX_USER ( data );

	tt_free_user ( user );

}


void tt_clear_friends ( GList *friends )
{
	if ( !friends ) return;

	// Libero memoria de structs amigos
	g_list_foreach ( friends, friend_list_remove, NULL );
	
	g_list_free ( friends );

}


void tt_set_networking ( TwiTux *twitter, gboolean flag )
{
	GtkWidget *detener;

	twitter->processing = flag;

	detener = twitter->principal->menuitem_detener;

	if ( detener && GTK_IS_WIDGET ( detener ) ) {
		gtk_widget_set_sensitive ( twitter->principal->menuitem_detener, flag );
	}

}


gchar *get_ruta_imagen ( const gchar *file )
{
	return g_build_path ( G_DIR_SEPARATOR_S, DATADIR, "pixmaps", file, NULL );
}
