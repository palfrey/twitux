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

#include <string.h>

#include <gtk/gtk.h>

#include <libsexy/sexy.h>

#include "network.h"
#include "main.h"
#include "callbacks.h"
#include "common.h"
#include "gcommon.h"
#include "gui.h"
#include "twitter.h"


#define TT_ANIMATION_STEP 100
#define TT_ANIMATION_STEP_INC 20

#define TT_EXPAND_OFFSET 55


static gboolean ocultar_timeout ( gpointer data );

static gboolean resize_timeout ( gpointer data );

static gint llevar_a;

static gboolean en_timeout = FALSE;


//-- Al destruirse la ventana
void tt_on_window_destroy ( GtkWidget *window, gpointer user_data )
{
	TwiTuxWindow *tt_window = TWITUX_TWITUX_WINDOW ( user_data );

	if ( !tt_window ) return;

	// Borro elementos existentes en la lista_tree
	tt_clear_list ( tt_window->lista_tree );
	
	gtk_widget_hide ( window );

	gtk_main_quit ();
	
}


//-- Al cerrar
void tt_on_cerrar ( GtkMenuItem *menuitem, gpointer user_data )
{

	gtk_widget_destroy ( GTK_WIDGET ( user_data ) );

}


//-- Al pedir el dialogo Acerca de
void tt_on_acerca_de ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTuxConfig *cfg = TWITUX_CONFIG ( user_data );
	GtkWidget *dialogo = tt_gui_create_dialogo_acerca ( cfg );

	gtk_dialog_run ( GTK_DIALOG ( dialogo ) );
	gtk_object_destroy ( GTK_OBJECT (dialogo) );

}


//-- Al hacer click en el link de 'Ver sitio de Twitux' en 'Acerca de..'
void tt_on_acerca_link ( GtkAboutDialog *dialog, const gchar *link, gpointer user_data )
{
	TwiTuxConfig *cfg = TWITUX_CONFIG ( user_data );

	tt_open_web_browser ( cfg->client, link );

}


//-- Al seleccionar conectarse
void tt_on_conectar ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter;
	GtkWidget *dialog;
	gint respuesta;
	GtkWidget *entry_usuario;
	GtkWidget *entry_password;
	GtkWidget *check_remember;

	twitter = TWITUX_TWITUX ( user_data );

	// Ya hay un proceso
	if ( twitter->processing ) return;

	// Creo el dialogo
	dialog = tt_gui_create_dialogo_login ( &entry_usuario, &entry_password, &check_remember , twitter );

	// Lo muestro y eso
	respuesta = gtk_dialog_run ( GTK_DIALOG  ( dialog ) );

	if ( respuesta == GTK_RESPONSE_ACCEPT ) {

		TwiTuxConfig *conf = twitter->gconf;

		const gchar *tmpuser = gtk_entry_get_text ( GTK_ENTRY ( entry_usuario ) );

		const gchar *tmppass = gtk_entry_get_text ( GTK_ENTRY ( entry_password ) );

		// Checkeo si completó los campos
		if ( !g_str_equal ( tmpuser, "" ) && !g_str_equal ( tmppass, "" ) ) {

			conf->user_passwd = g_strdup ( tmppass );

			conf->user_login = g_strdup ( tmpuser );

			conf->user_remember = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( check_remember ) );
			
			// Inicio session (pido lista de amigos)
			tt_network_login ( twitter );

		}

	}

	gtk_widget_destroy ( dialog );

}


//-- Al cambiar la opcion ver barra de estado
void tt_on_ver_estado ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	tt_ver_ocultar_widget ( GTK_WIDGET ( twitter->principal->barra_estado ),  ver );

	// Recordar en configuracion;
	twitter->gconf->ver_estado = ver;

}


//-- Al cambiar la opcion expandir mensajes
void tt_on_ver_expandir_mensajes ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	// Recordar en configuracion;
	twitter->gconf->expandir_mensajes = ver;

}


//-- Al seleccionar recargar timeline actual
void tt_on_recargar ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	if ( twitter->processing ) return;

	tt_timeout_remove ( twitter );
	
	tt_network_get_timeline ( twitter, twitter->current_timeline, FALSE );

}


//-- Al seleccionar desconectarse
void tt_on_desconectar ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	tt_cambiar_mensaje_estado ( twitter, _("Ok, logged out.") );

	// Reiniciar libsoup
	g_object_unref ( twitter->conexion );
	tt_network_load ( twitter );

	// Borro los amigos y reseteo el menu
	tt_new_friends_menu ( twitter );

	tt_clear_friends ( twitter->friends );

	twitter->friends = NULL;

	// Deshabilitar controles de amigos
	tt_enable_disable_widgets ( FALSE, twitter->principal );

	// Flag de iniciada sesion
	twitter->conectado = FALSE;

}


//-- Al seleccionar 'Establecer como home timeline'
void tt_on_establecer_home ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	if ( twitter->gconf->home_timeline ) {
		g_free ( twitter->gconf->home_timeline );
	}	

	twitter->gconf->home_timeline = g_strdup ( twitter->current_timeline );

	tt_cambiar_mensaje_estado ( twitter, _("Ok, new home timeline saved.") );
}


//-- Al ir al Home Timeline
void tt_on_ver_home_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;

	tt_network_get_timeline ( twitter, twitter->gconf->home_timeline, TRUE );

}


//-- Al seleccionar ver timeline de amigos
void tt_on_ver_amigos_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;

	tt_network_get_timeline ( twitter, TWITTER_FRIENDS_TIMELINE, TRUE );

}


//-- Al seleccionar ver mi timeline
void tt_on_ver_mi_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;
	
	tt_network_get_timeline ( twitter, TWITTER_MY_TIMELINE, TRUE );

}


//-- Al seleccionar ver respuestas
void tt_on_ver_respuestas ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;

	tt_network_get_timeline ( twitter, TWITTER_MY_REPLIES, TRUE );

}


//-- Al seleccionar ver el timeline publico
void tt_on_ver_public_timeline ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;

	tt_network_get_timeline ( twitter, TWITTER_PUBLIC_TIMELINE, TRUE );

}


//-- Al seleccionar ver timeline de un amigo
void tt_on_friend ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gchar *timeline;
	TwiTuxUser *user = NULL;

	// Obtengo el nombre de usuario
	g_object_get ( G_OBJECT ( menuitem ) , "user-data", &user, NULL );

	g_return_if_fail ( user != NULL );

	timeline = g_strdup_printf ( TWITTER_USER_TIMELINE, user->screen_name );
	// Cargo el timeline de usuario

	tt_network_get_timeline ( twitter, timeline, TRUE );

	g_free (timeline);

}


//-- Al seleccionar leer mensajes directos.
void tt_on_ver_mensajes_directos ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	if ( twitter->processing || !ver ) return;

	tt_network_direct_messages ( twitter );

}


//-- Al seleccionar "What are you doing?"
void tt_on_what ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX(user_data);

	GtkWidget *dialog;
	GtkTextBuffer *buffer;

	gint respuesta;

	TwiTuxTinyzer *tinyzer;

	// Ya hay un proceso
	if ( twitter->processing ) return;	

	// Tinyzer
	tinyzer = g_new0 (TwiTuxTinyzer, 1);

	tinyzer->twitter = twitter;

	// Creo el dialogo
	dialog = tt_gui_create_dialogo_what ( twitter, &buffer, tinyzer );

	// Lo muestro y eso
	respuesta = gtk_dialog_run ( GTK_DIALOG  (dialog) );

	if ( respuesta == GTK_RESPONSE_ACCEPT ) {

		GtkTextIter start_iter;
		GtkTextIter end_iter;
		gchar *text;
		
		gtk_text_buffer_get_start_iter ( buffer, &start_iter );
		gtk_text_buffer_get_end_iter ( buffer, &end_iter );

		if ( !gtk_text_iter_equal (&start_iter, &end_iter) ){

			text = gtk_text_buffer_get_text ( buffer, &start_iter, &end_iter, TRUE );

			tt_network_post_status ( twitter, text );

			g_free (text);

		}

	}

	g_free ( tinyzer );
	
	gtk_widget_destroy (dialog);

}


//-- Al detener el proceso actual
void tt_on_detener ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	
	soup_session_abort ( twitter->conexion );
}


//-- Al ver el timeline de Twitux
void tt_on_twitux ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	if ( twitter->processing ) return;

	tt_network_get_timeline ( twitter, TWITTER_TWITUX_TIMELINE, TRUE );

}


//-- Al reportar bugs
void tt_on_bugs ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTuxConfig *cfg = TWITUX_CONFIG ( user_data );

	tt_open_web_browser ( cfg->client, TWITTER_ABOUT_BUGZILLA );

}


//-- Al seleccionar 'enviar mensaje directo'
void tt_on_mensaje ( GtkMenuItem *menuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	GtkWidget *amigos;

	gint respuesta;

	// Ya hay un proceso
	if ( twitter->processing ) return;	

	// Creo el dialogo
	dialog = tt_gui_create_dialogo_direct ( twitter, &buffer, &amigos );

	// Cargo la lista de amigos
	g_list_foreach ( twitter->friends, tt_combo_amigos_add, amigos );

	// Lo muestro y eso
	respuesta = gtk_dialog_run ( GTK_DIALOG  (dialog) );

	if ( respuesta == GTK_RESPONSE_ACCEPT ) {
		gchar *friend;
		GtkTextIter start_iter;
		GtkTextIter end_iter;
		gchar *mensaje;

		friend = gtk_combo_box_get_active_text ( GTK_COMBO_BOX ( amigos ) );

		gtk_text_buffer_get_start_iter ( buffer, &start_iter );
		gtk_text_buffer_get_end_iter ( buffer, &end_iter );

		if ( !gtk_text_iter_equal (&start_iter, &end_iter) ){

			friend = gtk_combo_box_get_active_text ( GTK_COMBO_BOX ( amigos ) );

			if ( friend ) {

				mensaje = gtk_text_buffer_get_text ( buffer, &start_iter, &end_iter, TRUE );

				tt_network_send_message ( twitter, friend,mensaje );

				g_free (friend);

				g_free (mensaje);

			}

		}

	}

	gtk_widget_destroy (dialog);

}


//-- Al cerrar el expander de mensdajes
void tt_on_close_expand ( GtkButton *button, gpointer user_data )
{

	TwiTuxWindow *window = TWITUX_TWITUX_WINDOW ( user_data );

	if ( en_timeout ) return;

	en_timeout = TRUE;

	g_timeout_add ( TT_ANIMATION_STEP, ocultar_timeout, window->expand );

}


//-- Al hacer click en una url del expander
void tt_on_url_expand ( GtkWidget *url_label, gchar *url, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	gchar *timeline;

	//-- Abro la URL en el navegador
	if ( !strncmp (url, "http://", 7 ) || !strncmp( url, "ftp://", 6 ) ) {

		tt_open_web_browser ( twitter->gconf->client, url );

		return;

	}

	//-- Abro el timeline del usuario
	timeline = g_strdup_printf ( TWITTER_USER_TIMELINE, url );

	// Cargo el timeline de usuario
	tt_network_get_timeline ( twitter, timeline, TRUE );

	g_free (timeline);
	
}


//-- Al seleccionar un estado en la lista
void tt_on_status_changed ( GtkTreeView *treeview, gpointer user_data )
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	TwiTux *twitter;
	TwiTuxWindow *window;
	gchar *tmp;
	gchar *msg_status;
	TwiTuxStatus *status;

	GtkTreeSelection *sel = gtk_tree_view_get_selection ( treeview );

	twitter = TWITUX_TWITUX ( user_data );

	if ( !twitter->gconf->expandir_mensajes ) return;

	if ( !gtk_tree_selection_get_selected ( sel, &model, &iter ) ) return;

	window = twitter->principal;

	window  = twitter->principal;

	gtk_tree_model_get ( model, &iter, OBJECT_COLUMN, &status, -1 );

	//-- Cambio el autor
	tmp = g_strdup_printf ( "<b>%s</b>", status->user->name );

	gtk_label_set_markup ( GTK_LABEL ( window->expand->label_autor ), tmp );

	g_free ( tmp );

	//-- Cambio el mensaje
	tmp = g_strdup ( status->text );

	msg_status = xml_decode_alloc ( tmp );

	g_free ( tmp );

	tmp = tt_parse_urls_alloc ( msg_status );

	sexy_url_label_set_markup ( SEXY_URL_LABEL ( window->expand->sexy_label ), tmp );

	g_free ( tmp );

	g_free ( msg_status );

	// -- Expandir y animar panel
	if ( en_timeout ) return;

	// Hago que el SexyLabel se redibuje y obtener el .height nuevo
	gtk_widget_queue_draw ( window->expand->sexy_label );

	while (gtk_events_pending ()) gtk_main_iteration ();

	gtk_widget_realize ( window->expand->sexy_label );

	llevar_a = ( GTK_WIDGET ( window->expand->sexy_label )->allocation.height ) + TT_EXPAND_OFFSET;

	en_timeout = TRUE;

	g_timeout_add ( TT_ANIMATION_STEP, resize_timeout, window->expand->frame_expand );
	
}


//-- Al tinizar una URL
void tt_on_tinyze_url ( GtkWidget *widget, gpointer user_data )
{
	TwiTuxTinyzer *tinyzer = TWITUX_TINYZER ( user_data );

	const gchar *texto = gtk_entry_get_text ( GTK_ENTRY ( tinyzer->entry_long ) );

	if ( !texto ) return;

	sexy_url_label_set_markup ( SEXY_URL_LABEL ( tinyzer->entry_short ), _("Compressing URL...") );

	gtk_widget_set_sensitive ( tinyzer->entry_long, FALSE );

	gtk_widget_set_sensitive ( GTK_WIDGET ( tinyzer->button_ok ), FALSE );

	tt_network_tinyze ( tinyzer, texto );

}


//-- Al hacer click en el icono de notificacion
void on_icon_activate ( GtkStatusIcon *status_icon, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	GtkWidget *ventana = twitter->principal->ventana;

	if ( GTK_WIDGET_VISIBLE ( ventana ) ) {

		gtk_widget_hide_all ( ventana );

	} else {

		gtk_widget_show_all ( ventana );
		
	}

}


//-- Al mostrar el menu del area de notificacion
void on_icon_menu (GtkStatusIcon *status_icon, guint button,
							guint activate_time, gpointer user_data)
{
	GtkWidget* menu;
	GtkWidget* menu_item;

	TwiTux *twitter = TWITUX_TWITUX ( user_data );

	menu = gtk_menu_new ();

	menu_item = crear_menu ( _("What are you doing?"), menu );
	gtk_widget_set_sensitive ( menu_item, twitter->conectado );
	g_signal_connect ( menu_item, "activate", G_CALLBACK ( tt_on_what ), twitter );

	crear_menu_separador ( menu );

	menu_item = crear_stock_mi ( "gtk-about", menu, NULL );
	g_signal_connect ( menu_item, "activate", G_CALLBACK ( tt_on_acerca_de ), twitter->gconf );

	crear_menu_separador ( menu );

	menu_item = crear_stock_mi ( "gtk-quit", menu, NULL );
	g_signal_connect ( menu_item, "activate", G_CALLBACK ( tt_on_cerrar ), twitter->principal->ventana );

	gtk_widget_show_all ( menu );

	gtk_menu_popup(
			GTK_MENU ( menu ),
			NULL,
			NULL,
			gtk_status_icon_position_menu,
			status_icon,
			button,
			activate_time
	);
}


//-- Al hacer click en el boton cerrar de la ventana principal
gboolean tt_on_window_close_clicked ( GtkWidget *widget, GdkEvent *event, gpointer user_data )
{

	if ( gtk_status_icon_is_embedded ( GTK_STATUS_ICON ( user_data ) ) ){
		gtk_widget_hide ( widget );
		return TRUE;
	}

	return FALSE;

}


//-- Al cambiar la opcion de recargar timelines
void tt_on_recargar_timelines ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	// Recordar en configuracion;
	twitter->gconf->recargar_timelines = ver;

	// Si no quiero mas recargas, quito el timeout actual
	if ( !ver ) {

		tt_timeout_remove ( twitter );

	} else {

		// Si quiero timeouts, y NO estoy esperando alguna timeline
		if ( ! twitter->processing ) {

			tt_timeout_start ( twitter );

		}

	}

}


//-- Al cambiar la opcion de ver notificaciones
void tt_on_ver_burbujas ( GtkCheckMenuItem *checkmenuitem, gpointer user_data )
{
	TwiTux *twitter = TWITUX_TWITUX ( user_data );
	gboolean ver = gtk_check_menu_item_get_active ( checkmenuitem );

	// Recordar en configuracion;
	twitter->gconf->ver_burbujas = ver;
}



static gboolean resize_timeout ( gpointer data )
{

	GtkWidget *expander = GTK_WIDGET ( data );
	
	gint currentpost = expander->allocation.height;

	if ( llevar_a > currentpost ){

		currentpost += TT_ANIMATION_STEP_INC;

		if ( llevar_a < currentpost ){

			gtk_widget_set_size_request ( expander, -1, llevar_a );

		} else {

			gtk_widget_set_size_request ( expander, -1, currentpost );

			return TRUE;

		}

	} else if ( llevar_a < currentpost ){

		currentpost -= TT_ANIMATION_STEP_INC;

		if ( llevar_a > currentpost ){

			gtk_widget_set_size_request ( expander, -1, llevar_a );

		} else {

			gtk_widget_set_size_request ( expander, -1, currentpost );

			return TRUE;

		}

	}

	en_timeout = FALSE;

	return FALSE;

}


static gboolean ocultar_timeout ( gpointer data )
{
	TwiTuxExpand *expand = TWITUX_EXPANDER ( data );

	GtkWidget *expander = expand->frame_expand;

	gint currentpost = expander->allocation.height;

	currentpost -= TT_ANIMATION_STEP_INC;

	if ( currentpost < 1 ) {

		gtk_widget_set_size_request ( expander, -1, 1 );

		// Cambiar textos
		gtk_label_set_markup ( GTK_LABEL ( expand->label_autor ), "" );
		sexy_url_label_set_markup ( SEXY_URL_LABEL ( expand->sexy_label ), "" );

	} else if ( currentpost >= 1 ) {

		gtk_widget_set_size_request ( expander, -1, currentpost );

		return TRUE;

	}

	en_timeout = FALSE;

	return FALSE;

}
