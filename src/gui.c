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

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libsexy/sexy.h>
#include <libnotify/notify.h>

#include <libtwitux/twitux-conf.h>

#include "main.h"
#include "gui.h"
#include "common.h"
#include "gcommon.h"
#include "callbacks.h"
#include "network.h"
#include "twitter.h"
#include "twitux-preferences.h"

static GType g_iminfo_pointer_register (void);
static TwiTuxStatus *g_iminfo_pointer_copy ( gpointer *status );
static void g_iminfo_pointer_free ( gpointer *status );

GtkWidget *create_tinyzer ( TwiTuxTinyzer *tinyst );

#define TWITUX_TYPE_STATUS_POINTER g_iminfo_pointer_register()

// Agrega un amigo al combo del dialogo mensaje directo
void tt_combo_amigos_add ( gpointer data, gpointer userdata )
{
	GtkComboBox *combo;
	TwiTuxUser *user;

	if ( !data || !userdata )
		return;

	user = TWITUX_USER ( data );
	combo = GTK_COMBO_BOX ( userdata );

	gtk_combo_box_append_text ( combo, user->screen_name );

}


// Borra el menu anterior de amigos si existe, y borra el anterior
void tt_new_friends_menu ( TwiTux *twitter )
{
	GtkWidget *menu_friends;
	GtkWidget *menu_item_amigos;

	menu_item_amigos = twitter->principal->menuitem_amigos;

	if ( menu_item_amigos ){

		// Obtengo el menu amigos
		menu_friends = gtk_menu_item_get_submenu ( GTK_MENU_ITEM ( menu_item_amigos ) );

		// Lo borro
		if ( menu_friends ) {

			gtk_widget_destroy ( menu_friends );

		}

		// Creo un nuevo menu amigos
		menu_friends = gtk_menu_new ();
		gtk_menu_item_set_submenu ( GTK_MENU_ITEM ( menu_item_amigos ), menu_friends );

	}

}


// Agrega un amigo al menu amigos
void tt_add_friend ( TwiTux *twitter, TwiTuxUser *user )
{
	GtkWidget *menu_friends = gtk_menu_item_get_submenu ( GTK_MENU_ITEM ( twitter->principal->menuitem_amigos ) );
	GtkWidget *mi;

	g_return_if_fail ( menu_friends != NULL);

	// Agrego el amigo
	mi = crear_menu ( user->name,menu_friends );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_friend ), twitter );

	twitter->friends = g_list_append ( twitter->friends, user );

	g_object_set( mi, "user-data", user, NULL) ;

}


// Cambia la imagen de un statuse
void tt_set_pic ( GtkWidget *tree,  TwiTuxStatus *stauts, const gchar *file )
{
	GdkPixbuf *imagen;
	GtkListStore *store;

	// Cargo la imagen
	imagen = cargar_pixbuf ( file );
	if ( !imagen ) {

		g_unlink ( file );

		return;

	}

	// Cambio la imagen
	store = GTK_LIST_STORE ( gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree ) ) );
	gtk_list_store_set (store, &stauts->iter,  IMAGEN_COLUMN, imagen,  -1);

}


// Agrega un statuse al tree view
void tt_add_post ( TwiTuxWindow *window, TwiTuxStatus *stauts )
{
	GtkListStore *store;
	gchar *texto;
	gchar *status_tmp;
	gchar *status_msg;
	gchar *tmp_ruta;
	gchar *status_utf8;

	// Corto twitts largos
	if ( strlen (stauts->text) > 78 ){

		status_tmp = g_strndup ( stauts->text, 78 );

	} else {

		status_tmp = g_strdup ( stauts->text );

	}

	// Utf8 encodeo
	status_utf8 = g_locale_to_utf8 ( status_tmp, -1, NULL, NULL, NULL );

	// Decodeo
	status_msg = xml_decode_alloc ( status_utf8 );

	// Creo el texto con pango markup
	texto = g_strdup_printf ( "<span  size='small' foreground='black' weight='bold'>%s</span> - <span  size='small' foreground='gray' weight='bold'>%s</span>\n<span  size='small' foreground='darkblue'>%s..</span>", stauts->user->name, stauts->created_at, status_msg );

	store = GTK_LIST_STORE ( gtk_tree_view_get_model ( GTK_TREE_VIEW ( window-> lista_tree ) ) );

	gtk_list_store_append (store, &stauts->iter);
	
	tmp_ruta = get_ruta_imagen ( "twitux-loading.png" );
	gtk_list_store_set (store, &stauts->iter, 
						IMAGEN_COLUMN, cargar_pixbuf ( tmp_ruta ), 
						TEXTO_COLUMN, texto, 
						OBJECT_COLUMN,  stauts,
						-1);
	g_free ( tmp_ruta );

	g_free (texto);

	g_free (status_msg);

	g_free (status_tmp);

	g_free (status_utf8);

}


// Crea al treeview para mostrar statuses y mensajes
GtkWidget *crear_list_twitter ( TwiTux *twitter ) {
	// Cosas
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	// Widgets
	GtkWidget *tree_view;
	GtkListStore *store_tree;

	// Creo estructura para la lista
	store_tree = gtk_list_store_new ( N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, TWITUX_TYPE_STATUS_POINTER );

	// Creo el listado de amigos
	tree_view = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible ( GTK_TREE_VIEW ( tree_view ), FALSE );
	g_signal_connect ( tree_view, "cursor-changed", G_CALLBACK ( tt_on_status_changed ), twitter );

	gtk_tree_view_set_model ( GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (store_tree) );

	// Columna de imagenes
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes ( "Imagen", renderer, "pixbuf", IMAGEN_COLUMN, NULL );
	gtk_tree_view_append_column ( GTK_TREE_VIEW (tree_view ), column);

	// Columnas de texto
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ( "Texto", renderer, "markup", TEXTO_COLUMN, NULL );
	gtk_tree_view_append_column ( GTK_TREE_VIEW ( tree_view ), column);
	gtk_tree_view_column_set_expand (column, FALSE);
	g_object_set(renderer, "ypad", 0,"xpad", 5, "yalign", 0.0, "wrap-mode", PANGO_WRAP_WORD_CHAR, "wrap-width", 257, NULL);

	return tree_view;

}


// Crea la ventana principal
GtkWidget *tt_gui_ventana_principal ( TwiTux *twit )
{
	GtkWidget *ventana_principal;
	GtkWidget *mi;
	GtkWidget *mi_menu;
	GtkWidget *submenu;

	GtkWidget *misc;
	GtkWidget *lista_scroll;

	GtkAccelGroup *accel_group;

	gchar *titulo;
	gchar *tmp_ruta;

	GtkWidget *vbox7;
	GtkWidget *boton;
	GtkWidget *boton_imagen;

	GtkWidget *panel;
	GtkWidget *panel_aux;

	GSList *timelines = NULL;

	TwiTuxWindow *twitter = twit->principal;

	TwiTuxExpand *expand;

	TwituxConf *conf;
	gboolean expand_msg = TRUE;
	gboolean notify = TRUE;
	gboolean reload = TRUE;
	gboolean statusbar_visible = TRUE;

	// Atajos del teclados extra
	accel_group = gtk_accel_group_new ();

	titulo = g_strdup_printf ( "%s %s", TWITTER_HEADER_CLIENT, TWITTER_HEADER_VERSION );
	
	// Ventana principal :)
	ventana_principal = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_widget_set_size_request ( ventana_principal, 310, 475 );
	gtk_window_set_title ( GTK_WINDOW ( ventana_principal ), titulo );
	g_free ( titulo );
	gtk_window_set_position ( GTK_WINDOW ( ventana_principal ), GTK_WIN_POS_CENTER );
	tmp_ruta = get_ruta_imagen ( "twitux.png" );
	gtk_window_set_icon_from_file ( GTK_WINDOW (ventana_principal), tmp_ruta, NULL );
	g_free ( tmp_ruta );
	gtk_window_set_resizable ( GTK_WINDOW ( ventana_principal ), FALSE );

	// Señales de la ventana a capturar
	g_signal_connect ( ventana_principal, "destroy", G_CALLBACK ( tt_on_window_destroy ), twitter );
	g_signal_connect ( ventana_principal, "delete_event", G_CALLBACK ( tt_on_window_close_clicked ), twit->notify_icon );

	// Panel para empaquetar todo
	twitter->panel_principal = gtk_vbox_new ( FALSE, 0 );
	gtk_widget_show ( twitter->panel_principal );
	gtk_container_add ( GTK_CONTAINER ( ventana_principal ), twitter->panel_principal );

	// Barra de menu
	twitter->barra_menu = gtk_menu_bar_new ();
	gtk_widget_show ( twitter->barra_menu );
	gtk_box_pack_start ( GTK_BOX ( twitter->panel_principal ), twitter->barra_menu, FALSE, FALSE, 0 );

	// Menu Twitter
	mi = crear_menu ( _("_Twitter") , twitter->barra_menu );
	mi_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu ( GTK_MENU_ITEM (mi), mi_menu );
	twitter->menuitem_conectar = crear_stock_mi ( "gtk-connect", mi_menu, accel_group );
	g_signal_connect ( twitter->menuitem_conectar, "activate", G_CALLBACK ( tt_on_conectar ), twit );

	twitter->menuitem_desconectar = crear_stock_mi ("gtk-disconnect", mi_menu, accel_group );
	gtk_widget_set_sensitive ( twitter->menuitem_desconectar, FALSE );
	g_signal_connect ( twitter->menuitem_desconectar, "activate", G_CALLBACK ( tt_on_desconectar ), twit );

	misc = crear_menu_separador ( mi_menu );

	twitter->menuitem_enviar_estado = crear_menu ( _("What are you doing?"), mi_menu );
	gtk_widget_add_accelerator ( twitter->menuitem_enviar_estado, "activate", accel_group, GDK_T, (GdkModifierType) GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE );
	gtk_widget_set_sensitive ( twitter->menuitem_enviar_estado, FALSE );
	g_signal_connect ( twitter->menuitem_enviar_estado, "activate", G_CALLBACK ( tt_on_what ), twit );

	twitter->menuitem_enviar_mensaje = crear_menu ( _("Send a direct message..."), mi_menu );
	gtk_widget_set_sensitive ( twitter->menuitem_enviar_mensaje, FALSE );
	g_signal_connect ( twitter->menuitem_enviar_mensaje, "activate", G_CALLBACK ( tt_on_mensaje ), twit );

	misc = crear_menu_separador ( mi_menu );

	mi = crear_menu ( _("Set as home timeline"), mi_menu );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_establecer_home ), twit );

	misc = crear_menu_separador ( mi_menu );

	mi = crear_stock_mi ( "gtk-refresh", mi_menu, accel_group );
	gtk_widget_add_accelerator ( mi, "activate", accel_group, GDK_F5, (GdkModifierType) 0, GTK_ACCEL_VISIBLE );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_recargar ), twit );

	twitter->menuitem_detener = crear_stock_mi ( "gtk-stop", mi_menu, accel_group );
	gtk_widget_set_sensitive ( twitter->menuitem_detener, FALSE );
	g_signal_connect ( twitter->menuitem_detener, "activate", G_CALLBACK ( tt_on_detener ), twit );

	misc = crear_menu_separador ( mi_menu );

	mi = crear_stock_mi ( "gtk-quit", mi_menu, accel_group );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_cerrar ), ventana_principal );

	// Menu ver
	mi = crear_menu ( _("_View"), twitter->barra_menu );
	mi_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu ( GTK_MENU_ITEM ( mi ), mi_menu );

	mi = crear_menu_radio ( timelines, _("Home timeline"), TRUE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( mi ) );
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_ver_home_timeline ), twit );

	mi = crear_menu_radio ( timelines, _("Public timeline"), FALSE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( mi ) );
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_ver_public_timeline ), twit );

	twitter->menuitem_amigos_timeline = crear_menu_radio ( timelines, _("Friends timeline"), FALSE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( twitter->menuitem_amigos_timeline ) );
	gtk_widget_set_sensitive ( twitter->menuitem_amigos_timeline , FALSE );
	g_signal_connect ( twitter->menuitem_amigos_timeline, "toggled", G_CALLBACK ( tt_on_ver_amigos_timeline ), twit );

	misc = crear_menu_separador (mi_menu);

	// Sub menu amigos
	twitter->menuitem_amigos = crear_menu ( _("Friends"), mi_menu );
	gtk_widget_set_sensitive ( twitter->menuitem_amigos , FALSE );
	mi = gtk_menu_new ();
	gtk_menu_item_set_submenu ( GTK_MENU_ITEM ( twitter->menuitem_amigos ), mi );

	misc = crear_menu_separador ( mi_menu );

	twitter->menuitem_mensajes_directos = crear_menu_radio ( timelines, _("Direct messages"), FALSE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( twitter->menuitem_mensajes_directos ) );
	gtk_widget_set_sensitive ( twitter->menuitem_mensajes_directos , FALSE );
	g_signal_connect ( twitter->menuitem_mensajes_directos, "toggled", G_CALLBACK ( tt_on_ver_mensajes_directos ), twit );
	
	twitter->menuitem_respuestas = crear_menu_radio ( timelines, _("Replies"), FALSE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( twitter->menuitem_respuestas ) );
	gtk_widget_set_sensitive ( twitter->menuitem_respuestas , FALSE );
	g_signal_connect ( twitter->menuitem_respuestas, "toggled", G_CALLBACK ( tt_on_ver_respuestas ), twit );

	twitter->menuitem_mi_timeline = crear_menu_radio ( timelines, _("My timeline"), FALSE, mi_menu );
	timelines = gtk_radio_menu_item_get_group ( GTK_RADIO_MENU_ITEM ( twitter->menuitem_mi_timeline ) );
	gtk_widget_set_sensitive ( twitter->menuitem_mi_timeline , FALSE );
	g_signal_connect ( twitter->menuitem_mi_timeline, "toggled", G_CALLBACK ( tt_on_ver_mi_timeline ), twit );

	misc = crear_menu_separador ( mi_menu );

	// Submenu opciones avanzadas
	misc = crear_menu ( _("Set advanced options"), mi_menu );
	submenu = gtk_menu_new ();
	gtk_menu_item_set_submenu ( GTK_MENU_ITEM ( misc ), submenu );

	/* Let's get the gconf client. */
	conf = twitux_conf_get ();

	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_UI_EXPAND_MESSAGES,
						  &expand_msg);	
	mi = crear_menu_check ( submenu, _("Expand messages"), expand_msg);
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_ver_expandir_mensajes ), twit );

	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_UI_NOTIFICATION,
						  &notify);
	mi = crear_menu_check ( submenu, _("Show gui notifications"), notify);
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_ver_burbujas ), twit );

	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_TWEETS_RELOAD_TIMELINES,
						  &reload);
	mi = crear_menu_check ( submenu, _("Auto reload timelines"), reload);
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_recargar_timelines ), twit );

	misc = crear_menu_separador ( mi_menu );
	
	twitux_conf_get_bool (conf,
						  TWITUX_PREFS_UI_STATUSBAR,
						  &statusbar_visible);
	mi = crear_menu_check ( mi_menu, _("Status bar"), statusbar_visible);
	g_signal_connect ( mi, "toggled", G_CALLBACK ( tt_on_ver_estado ), twit );

	// Menu ayuda
	mi = crear_menu ( _("H_elp"), twitter->barra_menu );
	mi_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu ( GTK_MENU_ITEM ( mi ), mi_menu );

	mi = crear_menu ( "News and updates timeline", mi_menu );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_twitux ), twit );

	mi = crear_menu ( _("Report bugs..."), mi_menu );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_bugs ), NULL );

	misc = crear_menu_separador ( mi_menu );

	mi = crear_stock_mi ( "gtk-about", mi_menu, accel_group );
	gtk_widget_add_accelerator ( mi, "activate", accel_group, GDK_F1, (GdkModifierType) 0, GTK_ACCEL_VISIBLE );
	g_signal_connect ( mi, "activate", G_CALLBACK ( tt_on_acerca_de ), NULL);

	// Scroll vertical para la lista
	lista_scroll = gtk_scrolled_window_new ( NULL, NULL );
	gtk_widget_show ( lista_scroll );
	gtk_box_pack_start ( GTK_BOX ( twitter->panel_principal ), lista_scroll, TRUE, TRUE, 0 );
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW ( lista_scroll ), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );
	gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW ( lista_scroll ), GTK_SHADOW_IN );

	// Lista para mostrar statuses
	twitter->lista_tree = crear_list_twitter ( twit );
	gtk_widget_show ( twitter->lista_tree );
	gtk_container_add (GTK_CONTAINER ( lista_scroll ), twitter->lista_tree );

	// Expandir statuses ;)
	expand = twitter->expand;
	expand->frame_expand = gtk_frame_new ( NULL );
	gtk_widget_show ( expand->frame_expand );
	gtk_box_pack_start ( GTK_BOX ( twitter->panel_principal ), expand->frame_expand, FALSE, FALSE, 3 );
	gtk_container_set_border_width ( GTK_CONTAINER ( expand->frame_expand ), 1 );

	panel = gtk_hbox_new ( FALSE, 0 );
	gtk_widget_show ( panel );
	gtk_container_add ( GTK_CONTAINER ( expand->frame_expand ), panel );

	tmp_ruta = get_ruta_imagen ( "twitux.png" );
	expand->image = gtk_image_new_from_file ( tmp_ruta );
	g_free ( tmp_ruta );
	gtk_widget_show ( expand->image );
	gtk_box_pack_start ( GTK_BOX ( panel ), expand->image, FALSE, TRUE, 0 );
	gtk_misc_set_alignment ( GTK_MISC ( expand->image ), 0.5, 0 );
	gtk_misc_set_padding ( GTK_MISC ( expand->image ), 6, 6 );

	vbox7 = gtk_vbox_new ( FALSE, 0 );
	gtk_widget_show ( vbox7 );
	gtk_box_pack_start ( GTK_BOX ( panel ), vbox7, TRUE, TRUE, 0 );

	panel_aux = gtk_hbox_new ( FALSE, 0 );
	gtk_widget_show ( panel_aux );
	gtk_box_pack_start ( GTK_BOX ( vbox7 ), panel_aux, FALSE, FALSE, 0 );
	
	expand->label_autor = gtk_label_new ( NULL );
	gtk_widget_show ( expand->label_autor );
	gtk_box_pack_start ( GTK_BOX ( panel_aux ), expand->label_autor, FALSE, FALSE, 0 );
	gtk_label_set_use_markup ( GTK_LABEL ( expand->label_autor ), TRUE );
	gtk_misc_set_alignment ( GTK_MISC ( expand->label_autor ), 0, 0.5);
	gtk_misc_set_padding ( GTK_MISC ( expand->label_autor ), 0, 5);

	boton = gtk_button_new ();
	gtk_widget_show ( boton );
	g_signal_connect ( boton, "clicked", G_CALLBACK ( tt_on_close_expand ), twitter );
	boton_imagen = gtk_image_new_from_stock ( "gtk-close", GTK_ICON_SIZE_BUTTON );
	gtk_button_set_image ( GTK_BUTTON ( boton ), boton_imagen );
	gtk_box_pack_end ( GTK_BOX ( panel_aux ), boton, FALSE, FALSE, 0 );
	gtk_button_set_relief ( GTK_BUTTON ( boton ), GTK_RELIEF_NONE );
	gtk_button_set_focus_on_click ( GTK_BUTTON ( boton ), FALSE);

	expand->sexy_label = sexy_url_label_new ();
	gtk_widget_show ( expand->sexy_label );
	sexy_url_label_set_markup ( SEXY_URL_LABEL ( expand->sexy_label ), NULL );
	g_signal_connect (expand->sexy_label, "url-activated", G_CALLBACK ( tt_on_url_expand ), twit );
	gtk_box_pack_start ( GTK_BOX ( vbox7 ), expand->sexy_label, FALSE, FALSE, 0 );
	gtk_label_set_line_wrap ( GTK_LABEL ( expand->sexy_label ), TRUE );
	gtk_misc_set_alignment ( GTK_MISC ( expand->sexy_label ), 0, 0.5 );

	// Barra de estado
	twitter->barra_estado = gtk_statusbar_new ();

	if (statusbar_visible) {
		gtk_widget_show ( twitter->barra_estado );
	}
	gtk_box_pack_start ( GTK_BOX ( twitter->panel_principal ), twitter->barra_estado, FALSE, FALSE, 0 );

	// Contexto para cambiar el msj de la barra de estado
	twitter->barra_estado_cid = gtk_statusbar_get_context_id ( GTK_STATUSBAR ( twitter->barra_estado ) , "TTSB" );

	// Agrego los metodos avebiados a la ventana
	gtk_window_add_accel_group ( GTK_WINDOW ( ventana_principal ), accel_group );

	return ventana_principal;

}


// Dialogo para iniciar sesion
GtkWidget *tt_gui_create_dialogo_login ( GtkWidget **entry_user, GtkWidget **entry_passwd, GtkWidget **check_remember, TwiTux *twitter )
{
	GtkWidget *dialogo_login;
	GtkWidget *dialog_vbox1;
	GtkWidget *box_horizontal;
	GtkWidget *banner_twitter;
	GtkWidget *alignment2;
	GtkWidget *framelogin;
	GtkWidget *alignment1;
	GtkWidget *box_datos;
	GtkWidget *label_usuario;
	GtkWidget *label_clave;
	GtkWidget *label1;

	TwituxConf *conf;
	gchar *user_id;
	gchar *user_passwd;
	gboolean auth_remember = FALSE;

	gchar *titulo = g_strdup_printf ( _("%s - Log in"), TWITTER_HEADER_CLIENT );
	gchar *tmp_ruta;

	conf = twitux_conf_get ();

	twitux_conf_get_bool (conf, TWITUX_PREFS_AUTH_REMEMBER, &auth_remember);

	dialogo_login = gtk_dialog_new_with_buttons ( titulo ,
											GTK_WINDOW ( twitter->principal->ventana ),
											GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_STOCK_CANCEL,
											GTK_RESPONSE_REJECT,
											GTK_STOCK_OK,
											GTK_RESPONSE_ACCEPT,
											NULL );

	g_free ( titulo );

	dialog_vbox1 = GTK_DIALOG ( dialogo_login )->vbox;
	gtk_widget_show ( dialog_vbox1 );

	box_horizontal = gtk_hbox_new ( FALSE, 0 );
	gtk_widget_show ( box_horizontal );
	gtk_box_pack_start ( GTK_BOX ( dialog_vbox1 ), box_horizontal, TRUE, TRUE, 0 );

	tmp_ruta = get_ruta_imagen ( "twitux-side.png" );
	banner_twitter = gtk_image_new_from_file ( tmp_ruta );
	g_free ( tmp_ruta );
	gtk_widget_show ( banner_twitter );
	gtk_box_pack_start ( GTK_BOX ( box_horizontal ), banner_twitter, TRUE, TRUE, 0 );
	gtk_misc_set_padding ( GTK_MISC ( banner_twitter ), 9, 10 );

	alignment2 = gtk_alignment_new ( 0.5, 0.5, 1, 1 );
	gtk_widget_show ( alignment2 );
	gtk_box_pack_start ( GTK_BOX ( box_horizontal ), alignment2, TRUE, TRUE, 0 );

	framelogin = gtk_frame_new ( NULL );
	gtk_widget_show ( framelogin );
	gtk_container_add ( GTK_CONTAINER ( alignment2) , framelogin );
	gtk_widget_set_size_request ( framelogin, 338, -1 );
	gtk_container_set_border_width ( GTK_CONTAINER ( framelogin ), 13 );

	alignment1 = gtk_alignment_new ( 0.5, 0.5, 1, 1 );
	gtk_widget_show ( alignment1 );
	gtk_container_add ( GTK_CONTAINER ( framelogin ), alignment1 );
	gtk_alignment_set_padding ( GTK_ALIGNMENT ( alignment1 ), 0, 0, 12, 0 );

	box_datos = gtk_vbox_new ( FALSE, 3 );
	gtk_widget_show ( box_datos );
	gtk_container_add ( GTK_CONTAINER ( alignment1 ), box_datos );
	gtk_container_set_border_width ( GTK_CONTAINER ( box_datos ), 11 );

	label_usuario = gtk_label_new ( _("<b>Username or e-mail:</b>") );
	gtk_widget_show ( label_usuario );
	gtk_box_pack_start ( GTK_BOX ( box_datos ), label_usuario, FALSE, FALSE, 0 );
	gtk_label_set_use_markup ( GTK_LABEL ( label_usuario ), TRUE );
	gtk_misc_set_alignment ( GTK_MISC ( label_usuario ), 0.04, 0.5 );
	gtk_misc_set_padding ( GTK_MISC ( label_usuario ), 0, 4 );

	*entry_user = gtk_entry_new ();
	gtk_widget_show ( *entry_user );
	gtk_box_pack_start ( GTK_BOX ( box_datos ), *entry_user, FALSE, FALSE, 0 );
	/* get the user login id from gconf if available */
	if ( auth_remember ) {
		twitux_conf_get_string (conf, TWITUX_PREFS_AUTH_USER_ID, &user_id);
		if (user_id) {
			gtk_entry_set_text (GTK_ENTRY (*entry_user), user_id);
			g_free (user_id);
		}
	}

	label_clave = gtk_label_new ( _("<b>Password:</b>") );
	gtk_widget_show ( label_clave );
	gtk_box_pack_start ( GTK_BOX ( box_datos ), label_clave, FALSE, FALSE, 0 );
	gtk_label_set_use_markup ( GTK_LABEL ( label_clave ), TRUE );
	gtk_misc_set_alignment ( GTK_MISC ( label_clave ), 0.04, 0.5 );
	gtk_misc_set_padding ( GTK_MISC ( label_clave ), 0, 4 );

	*entry_passwd = gtk_entry_new ();
	gtk_widget_show ( *entry_passwd );
	gtk_box_pack_start ( GTK_BOX ( box_datos ), *entry_passwd, FALSE, FALSE, 0 );
	gtk_entry_set_visibility (GTK_ENTRY ( *entry_passwd ), FALSE );
	/* Get the user's password from gconf if it's available */
	if( auth_remember ) {
		twitux_conf_get_string (conf, TWITUX_PREFS_AUTH_PASSWORD,
								&user_passwd);
		if (user_passwd) {
			gtk_entry_set_text (GTK_ENTRY (*entry_passwd ), user_passwd);
			g_free (user_passwd);
		}
	}

	*check_remember = gtk_check_button_new_with_mnemonic ( _("Remember user data") );
	gtk_widget_show ( *check_remember );
	gtk_box_pack_start (GTK_BOX ( box_datos ), *check_remember, FALSE, FALSE, 0 );
	gtk_container_set_border_width ( GTK_CONTAINER ( *check_remember ), 11 );
	/* Check gconf to see if we should remember the login.  Default is FALSE. */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (*check_remember), auth_remember);

	label1 = gtk_label_new ( _("Twitter login") );
	gtk_widget_show ( label1 );
	gtk_frame_set_label_widget ( GTK_FRAME ( framelogin ), label1 );
	gtk_label_set_use_markup ( GTK_LABEL ( label1 ), TRUE );
	gtk_misc_set_padding ( GTK_MISC ( label1 ), 12, 0 );

	return dialogo_login;

}


// Crear dialogo Acerca de..
GtkWidget *tt_gui_create_dialogo_acerca (void)
{
	GtkWidget *dialogo_acerca;
	const gchar *authors[] = {
		"Daniel Morales <daniel@suruware.com>",
		NULL
	};

	const gchar *gpl = _("This program is free software; you can redistribute it \n"
	"and/or modify it under the terms of the GNU General Public License\n"
	"as published by the Free Software Foundation; either version 2 of\n"
	"the License, or (at your option) any later version."
	"\n\nThis program is distributed in the hope that it will be useful,\n"
	"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"GNU General Public License for more details. You should have\n"
	"received a copy of the GNU General Public License along with\n"
	"this program; if not, write to the "
	"Free Software Foundation, Inc., \n59 Temple Place, Suite 330, \n"
	"Boston, MA  02111-1307  USA");

	gchar *tmp_ruta = get_ruta_imagen ( "twitux-top.png" );

	dialogo_acerca = gtk_about_dialog_new ();
	gtk_about_dialog_set_url_hook ( (GtkAboutDialogActivateLinkFunc) tt_on_acerca_link, NULL, NULL );
	gtk_about_dialog_set_logo ( GTK_ABOUT_DIALOG ( dialogo_acerca ), gdk_pixbuf_new_from_file ( tmp_ruta, NULL ) );
	gtk_about_dialog_set_version ( GTK_ABOUT_DIALOG ( dialogo_acerca ), TWITTER_HEADER_VERSION );
	gtk_about_dialog_set_name ( GTK_ABOUT_DIALOG ( dialogo_acerca ), TWITTER_HEADER_CLIENT );
	gtk_about_dialog_set_copyright ( GTK_ABOUT_DIALOG ( dialogo_acerca ), "Copyright © 2007 Daniel Morales" );
	gtk_about_dialog_set_comments ( GTK_ABOUT_DIALOG ( dialogo_acerca ), _("A Twitter client for the Gnome desktop") );
	gtk_about_dialog_set_license ( GTK_ABOUT_DIALOG ( dialogo_acerca ), gpl );
	gtk_about_dialog_set_website ( GTK_ABOUT_DIALOG ( dialogo_acerca ), "http://twitux.sourceforge.net" );
	gtk_about_dialog_set_website_label ( GTK_ABOUT_DIALOG ( dialogo_acerca ), _("Visit Twitux web site") );
	gtk_about_dialog_set_authors ( GTK_ABOUT_DIALOG ( dialogo_acerca ), authors );
	gtk_about_dialog_set_translator_credits ( GTK_ABOUT_DIALOG ( dialogo_acerca ), _("translator-credits") );

	g_free ( tmp_ruta );

	return dialogo_acerca;

}


// Crear el dialogo para enviar estados
GtkWidget *tt_gui_create_dialogo_what ( TwiTux *twitter, GtkTextBuffer **buffer, TwiTuxTinyzer *tinyst )
{
	GtkWidget *dialogo;
	GtkWidget *dialog_vbox1;
	
	GtkWidget *frame1;
	GtkWidget *alignment1;
	GtkWidget *vbox1;
	GtkWidget *image1;
	GtkWidget *scrolledwindow1;
	GtkWidget *textview1;
	GtkWidget *label;

	GtkWidget *tinyzer;

	gchar *titulo = g_strdup_printf ( _("%s - What are you doing?"), TWITTER_HEADER_CLIENT );

	gchar *tmp_ruta;

	dialogo = gtk_dialog_new_with_buttons ( titulo ,
												GTK_WINDOW ( twitter->principal->ventana ),
												GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_STOCK_CANCEL,
												GTK_RESPONSE_REJECT,
												GTK_STOCK_OK,
												GTK_RESPONSE_ACCEPT,
												NULL);

	g_free ( titulo );

	gtk_window_set_resizable ( GTK_WINDOW ( dialogo ), FALSE );

	dialog_vbox1 = GTK_DIALOG ( dialogo )->vbox;
	gtk_widget_show ( dialog_vbox1 );
	
	frame1 = gtk_frame_new ( NULL );
	gtk_widget_show ( frame1 );
	gtk_box_pack_start ( GTK_BOX ( dialog_vbox1 ), frame1, TRUE, TRUE, 0 );
	gtk_frame_set_label_align ( GTK_FRAME ( frame1 ), 0.5, 0.5 );
	gtk_frame_set_shadow_type ( GTK_FRAME ( frame1 ), GTK_SHADOW_IN );

	alignment1 = gtk_alignment_new ( 0.5, 0.5, 1, 1 );
	gtk_widget_show ( alignment1 );
	gtk_container_add ( GTK_CONTAINER ( frame1 ), alignment1 );
	gtk_widget_set_size_request ( alignment1, 330, -1 );
	gtk_alignment_set_padding ( GTK_ALIGNMENT ( alignment1 ), 5, 5, 5, 5 );

	vbox1 = gtk_vbox_new ( FALSE, 0 );
	gtk_widget_show ( vbox1 );
	gtk_container_add ( GTK_CONTAINER ( alignment1 ), vbox1 );

	tmp_ruta = get_ruta_imagen ( "twitux-top.png" );
	image1 = gtk_image_new_from_file ( tmp_ruta );
	g_free ( tmp_ruta );
	gtk_widget_show ( image1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), image1, FALSE, FALSE, 0 );
	gtk_misc_set_padding ( GTK_MISC ( image1 ), 0, 6 );

	label = gtk_label_new ( _("<b>What are you doing?</b>") );
	gtk_widget_show ( label );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), label, FALSE, FALSE, 0 );
	gtk_label_set_use_markup ( GTK_LABEL ( label ), TRUE );
	gtk_misc_set_alignment ( GTK_MISC ( label ), 0.5, 0.5 );
	gtk_misc_set_padding ( GTK_MISC ( label ), 0, 4 );

	scrolledwindow1 = gtk_scrolled_window_new ( NULL, NULL );
	gtk_widget_show ( scrolledwindow1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), scrolledwindow1, TRUE, TRUE, 0 );
	gtk_container_set_border_width ( GTK_CONTAINER ( scrolledwindow1 ), 4 );
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW ( scrolledwindow1 ), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );
	gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW ( scrolledwindow1 ), GTK_SHADOW_IN );

	textview1 = gtk_text_view_new ();
	gtk_widget_show ( textview1 );
	gtk_container_add ( GTK_CONTAINER ( scrolledwindow1 ), textview1 );
	gtk_text_view_set_wrap_mode ( GTK_TEXT_VIEW ( textview1 ), GTK_WRAP_WORD );
	gtk_text_view_set_left_margin ( GTK_TEXT_VIEW ( textview1 ), 2 );
	gtk_text_view_set_right_margin ( GTK_TEXT_VIEW ( textview1 ), 2 );

	*buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( textview1 ) );

	tinyzer = create_tinyzer ( tinyst );

	gtk_box_pack_end ( GTK_BOX ( vbox1 ), tinyzer, TRUE, FALSE, 0 );

	gtk_widget_grab_focus ( textview1 );

	return dialogo;

}


// Dialogo para enviar mensajes directos
GtkWidget *tt_gui_create_dialogo_direct ( TwiTux *twitter, GtkTextBuffer **buffer, GtkWidget **amigos )
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *frame1;
	GtkWidget *alignment1;
	GtkWidget *vbox1;
	GtkWidget *image1;
	GtkWidget *label2;
	GtkWidget *combobox1;
	GtkWidget *label1;
	GtkWidget *scrolledwindow1;
	GtkWidget *textview1;

	gchar *titulo = g_strdup_printf ( _("%s - Send a direct message"), TWITTER_HEADER_CLIENT ); 

	gchar *tmp_ruta;

	dialog1 = gtk_dialog_new_with_buttons ( titulo ,
											GTK_WINDOW ( twitter->principal->ventana ),
											GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
											GTK_STOCK_CANCEL,
											GTK_RESPONSE_REJECT,
											GTK_STOCK_OK,
											GTK_RESPONSE_ACCEPT,
											NULL );

	g_free ( titulo );

	gtk_window_set_resizable ( GTK_WINDOW ( dialog1 ), FALSE );

	dialog_vbox1 = GTK_DIALOG ( dialog1 )->vbox;
	gtk_widget_show ( dialog_vbox1 );

	frame1 = gtk_frame_new ( NULL );
	gtk_widget_show ( frame1 );
	gtk_box_pack_start ( GTK_BOX ( dialog_vbox1 ), frame1, TRUE, TRUE, 0 );
	gtk_frame_set_label_align ( GTK_FRAME ( frame1 ), 0.5, 0.5 );
	gtk_frame_set_shadow_type ( GTK_FRAME ( frame1 ), GTK_SHADOW_IN );

	alignment1 = gtk_alignment_new ( 0.5, 0.5, 1, 1 );
	gtk_widget_show ( alignment1 );
	gtk_container_add ( GTK_CONTAINER (frame1), alignment1 );
	gtk_widget_set_size_request ( alignment1, 330, 220 );
	gtk_alignment_set_padding ( GTK_ALIGNMENT ( alignment1 ), 5, 5, 5, 5 );

	vbox1 = gtk_vbox_new ( FALSE, 0 );
	gtk_widget_show ( vbox1 );
	gtk_container_add ( GTK_CONTAINER ( alignment1 ), vbox1 );

	tmp_ruta = get_ruta_imagen ( "twitux-top.png" );
	image1 = gtk_image_new_from_file ( tmp_ruta );
	g_free ( tmp_ruta );
	gtk_widget_show ( image1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), image1, FALSE, FALSE, 0 );
	gtk_misc_set_padding ( GTK_MISC ( image1 ), 0, 6 );

	label2 = gtk_label_new ( _("<b>Select a friend:</b>") );
	gtk_widget_show ( label2 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), label2, FALSE, FALSE, 2 );
	gtk_label_set_use_markup ( GTK_LABEL ( label2 ), TRUE );

	combobox1 = gtk_combo_box_new_text ();
	gtk_widget_show ( combobox1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), combobox1, TRUE, TRUE, 4 );

	label1 = gtk_label_new ( _("<b>Write the message:</b>") );
	gtk_widget_show ( label1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), label1, FALSE, FALSE, 2 );
	gtk_label_set_use_markup ( GTK_LABEL ( label1 ), TRUE );

	scrolledwindow1 = gtk_scrolled_window_new ( NULL, NULL );
	gtk_widget_show ( scrolledwindow1 );
	gtk_box_pack_start ( GTK_BOX ( vbox1 ), scrolledwindow1, TRUE, TRUE, 0 );
	gtk_container_set_border_width ( GTK_CONTAINER ( scrolledwindow1 ), 4 );
	gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW ( scrolledwindow1 ), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );
	gtk_scrolled_window_set_shadow_type ( GTK_SCROLLED_WINDOW ( scrolledwindow1 ), GTK_SHADOW_IN );

	textview1 = gtk_text_view_new ();
	gtk_widget_show ( textview1 );
	gtk_container_add ( GTK_CONTAINER ( scrolledwindow1 ), textview1 );
	gtk_text_view_set_wrap_mode ( GTK_TEXT_VIEW ( textview1 ), GTK_WRAP_WORD );

	*buffer = gtk_text_view_get_buffer ( GTK_TEXT_VIEW ( textview1 ) );
	*amigos = combobox1;
	
	return dialog1;

}


GtkWidget *create_tinyzer ( TwiTuxTinyzer *tinyst )
{
	GtkWidget *tiny_expander;
	GtkWidget *vbox3;
	
	GtkWidget *tiny_label_url;
	GtkWidget *hbox1;
	GtkWidget *tiny_entry_url;
	GtkWidget *tiny_boton;
	GtkWidget *tiny_label_tiny;

	GtkWidget *tiny_title;
	
	tiny_expander = gtk_expander_new (NULL);
	gtk_widget_show (tiny_expander);
	gtk_container_set_border_width (GTK_CONTAINER (tiny_expander), 3);

	vbox3 = gtk_vbox_new (FALSE, 3);
	gtk_widget_show (vbox3);
	gtk_container_add (GTK_CONTAINER (tiny_expander), vbox3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox3), 5);

	tiny_label_url = gtk_label_new (_("Url to tinyze:"));
	gtk_widget_show (tiny_label_url);
	gtk_box_pack_start (GTK_BOX (vbox3), tiny_label_url, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (tiny_label_url), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (tiny_label_url), 5, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox3), hbox1, FALSE, FALSE, 0);

	tiny_entry_url = gtk_entry_new ();
	gtk_widget_show (tiny_entry_url);
	gtk_box_pack_start (GTK_BOX (hbox1), tiny_entry_url, TRUE, TRUE, 0);
	g_signal_connect ( tiny_entry_url, "activate", G_CALLBACK ( tt_on_tinyze_url ), tinyst );
	
	tiny_boton = gtk_button_new_with_mnemonic (_("Ok"));
	g_signal_connect (tiny_boton, "clicked", G_CALLBACK ( tt_on_tinyze_url ), tinyst );
	gtk_widget_show (tiny_boton);
	gtk_box_pack_start (GTK_BOX (hbox1), tiny_boton, FALSE, FALSE, 0);

	tiny_label_tiny = sexy_url_label_new ();
	gtk_widget_show ( tiny_label_tiny );
	sexy_url_label_set_markup ( SEXY_URL_LABEL (tiny_label_tiny), "Powered by: <a href='http://www.tinyurl.com'>Tinyurl.com</a>" );
	g_signal_connect ( tiny_label_tiny, "url-activated", G_CALLBACK ( tt_on_url_expand ), tinyst->twitter );
	gtk_label_set_selectable ( GTK_LABEL (tiny_label_tiny), TRUE );
	gtk_misc_set_alignment (GTK_MISC (tiny_label_tiny), 0, 0.5);
	gtk_misc_set_padding (GTK_MISC (tiny_label_tiny), 5, 0);

	gtk_box_pack_start ( GTK_BOX ( vbox3 ), tiny_label_tiny, FALSE, FALSE, 0 );

	tiny_title = gtk_label_new ( "<b>URL Tinyzer</b>" );
	gtk_widget_show (tiny_title);
	gtk_expander_set_label_widget (GTK_EXPANDER (tiny_expander), tiny_title);
	gtk_label_set_use_markup (GTK_LABEL (tiny_title), TRUE);

	tinyst->entry_long = tiny_entry_url;
	tinyst->entry_short = tiny_label_tiny;
	tinyst->button_ok = tiny_boton;

	return tiny_expander;

}


GtkStatusIcon *tt_gui_create_notify_icon ( TwiTux *twitter )
{
	gchar *tmp_ruta;
	GtkStatusIcon *icono;

	tmp_ruta = get_ruta_imagen ( "twitux.png" );
	icono = gtk_status_icon_new_from_file ( tmp_ruta );
	g_free ( tmp_ruta );

	gtk_status_icon_set_visible ( icono, TRUE );
	gtk_status_icon_set_tooltip ( icono, "Twitux" );
	g_signal_connect ( icono, "activate", G_CALLBACK ( on_icon_activate ), twitter );
	g_signal_connect ( icono, "popup-menu", G_CALLBACK ( on_icon_menu ), twitter );
	
	return icono;
}


void tt_gui_show_bubble ( TwiTux *twitter, const char *mensaje )
{
	GdkPixbuf *pixbuf;
	gint icon_width;
	gint icon_height;
	gchar *tmpfile;

	// Si no hay icono en el area de notificacion, no se muestra
	if ( ! gtk_status_icon_is_embedded ( twitter->notify_icon ) ) {

		return;

	}

	// Por si ya hay n cartelito
	if ( twitter->bubble != NULL ) {
		notify_notification_close ( twitter->bubble, NULL );
	}

	twitter->bubble = notify_notification_new_with_status_icon ( TWITTER_HEADER_CLIENT, mensaje, NULL, twitter->notify_icon );
	
	notify_notification_set_timeout ( twitter->bubble, 3000 );

	gtk_icon_size_lookup ( GTK_ICON_SIZE_LARGE_TOOLBAR, &icon_width, &icon_height );

	tmpfile = get_ruta_imagen ( "twitux.png" );
	pixbuf =  gdk_pixbuf_new_from_file_at_size ( tmpfile, icon_width, icon_height, NULL );
	g_free ( tmpfile );

	if ( pixbuf ) {
		notify_notification_set_icon_from_pixbuf ( twitter->bubble, pixbuf );
	}

	if ( !notify_notification_show ( twitter->bubble, NULL ) ) {
		g_warning ( "failed to send notification bubble" );
	}

}


static void g_iminfo_pointer_free ( gpointer *status )
{
	TwiTuxStatus *ptr;

	g_return_if_fail ( status != NULL );

	ptr = TWITUX_STATUS ( status );

	// Free user stuff..
	tt_free_user ( ptr->user );

	// Free status..
	tt_free_status ( ptr ); 
}


//
static TwiTuxStatus *g_iminfo_pointer_copy ( gpointer *status )
{
	TwiTuxStatus	*ptr;
	TwiTuxStatus	*newp;

	g_return_val_if_fail( status!= NULL , NULL);

	ptr = TWITUX_STATUS ( status );
	newp = g_new0 (TwiTuxStatus, 1);

	// Copy status stuff..
	if ( ptr->text ) newp->text = g_strdup ( ptr->text );
	if ( ptr->id ) newp->id = g_strdup ( ptr->id );
	if ( ptr->tree ) newp->tree = ptr->tree;
	if ( ptr->created_at ) newp->created_at = g_strdup ( ptr->created_at );
	newp->iter = ptr->iter;
	
	// Copy user stuff
	if ( ptr->user ) {
		newp->user = g_new0 (TwiTuxUser, 1);
		if ( ptr->user->id ) newp->user->id = g_strdup (ptr->user->id );
		if ( ptr->user->screen_name ) newp->user->screen_name = g_strdup (ptr->user->screen_name );
		if ( ptr->user->name ) newp->user->name = g_strdup (ptr->user->name );
		if ( ptr->user->profile_image_url ) newp->user->profile_image_url = g_strdup (ptr->user->profile_image_url );
		if ( ptr->user->profile_image_md5 ) newp->user->profile_image_md5 = g_strdup (ptr->user->profile_image_md5 );
	}

	return newp;

}


//
static GType g_iminfo_pointer_register ( void )
{
	static GType iminfo_pointer_type = 0;

	if (!iminfo_pointer_type)
		iminfo_pointer_type =
			g_boxed_type_register_static
				("TWITUX_TYPE_STATUS_POINTER",
					(GBoxedCopyFunc) g_iminfo_pointer_copy,
					(GBoxedFreeFunc) g_iminfo_pointer_free);

	return iminfo_pointer_type;

}
