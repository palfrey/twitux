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

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libtwitux/twitux-debug.h>

#include "twitux.h"
#include "twitux-send-message-dialog.h"
#include "twitux-spell-dialog.h"
#include "twitux-glade.h"
#include "twitux-network.h"

#define DEBUG_DOMAIN    "SendMessage"

/* Let's use the preferred maximum character count */
#define MAX_CHARACTER_COUNT 140

typedef struct {
	GtkWidget *dialog;
	GtkWidget *textview;
	GtkWidget *label;
	GtkWidget *send_button;
} TwituxSendMessage;

typedef struct {
	GtkWidget         *textview;
	gchar             *word;

	GtkTextIter        start;
	GtkTextIter        end;
} TwituxMessageSpell;

static void message_set_characters_available    (GtkTextBuffer      *buffer,
												 TwituxSendMessage  *send_message);
static void message_text_buffer_changed_cb      (GtkTextBuffer      *buffer,
											     TwituxSendMessage  *send_message);
static void message_text_populate_popup_cb      (GtkTextView        *view,
												 GtkMenu            *menu,
												 TwituxSendMessage  *send_message);
static void message_text_check_word_spelling_cb (GtkMenuItem *menuitem,
												 TwituxMessageSpell *message_spell);
static TwituxMessageSpell *message_spell_new    (GtkWidget          *window,
											     const gchar        *word,
											     GtkTextIter         start,
											     GtkTextIter         end);
static void message_spell_free                  (TwituxMessageSpell *message_spell);
static void message_destroy_cb                  (GtkWidget          *widget,
											     TwituxSendMessage  *send_message);
static void message_response_cb                 (GtkWidget          *widget,
											     gint                response,
											     TwituxSendMessage  *send_message);

gchar *
url_encode_message (gchar *text)
{
	const char        *good;
	static const char  hex[16] = "0123456789ABCDEF";
	GString           *result;

	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (*text != '\0', NULL);

	/* RFC 3986 */ 
	good = "~-._";

	result = g_string_new (NULL);
	while (*text) {
		unsigned char c = *text++;

		if (g_ascii_isalnum (c) || strchr (good, c))
			g_string_append_c (result, c);
		else {
			g_string_append_c (result, '%');
			g_string_append_c (result, hex[c >> 4]);
			g_string_append_c (result, hex[c & 0xf]);
		}
	}

	return g_string_free (result, FALSE);
}

static void
message_set_characters_available (GtkTextBuffer     *buffer,
								  TwituxSendMessage *send_message)
{
	gint i;
	gint count;
	const gchar *standard_msg;
	gchar *character_count;

	i = gtk_text_buffer_get_char_count (buffer);

	count = MAX_CHARACTER_COUNT - i;
	if (count < 0) {
		count = 0;
	}

	standard_msg = _("Characters Available");

	if (i > MAX_CHARACTER_COUNT) {
		character_count =
			g_markup_printf_escaped ("<span size=\"small\">%s: <span foreground=\"red\">%i</span></span>",
									 standard_msg, count);
		gtk_widget_set_sensitive (send_message->send_button, FALSE);
	} else {
		character_count =
			g_markup_printf_escaped ("<span size=\"small\">%s: <span foreground=\"black\">%i</span></span>",
									 standard_msg, count);
		gtk_widget_set_sensitive (send_message->send_button, TRUE);
	}

	gtk_label_set_markup (GTK_LABEL (send_message->label), character_count);
	g_free (character_count);
}

static void
message_text_buffer_changed_cb (GtkTextBuffer     *buffer,
								TwituxSendMessage *send_message)
{
	GtkTextIter  start;
	GtkTextIter  end;
	gchar       *str;
	gboolean     spell_checker = FALSE;


	message_set_characters_available (buffer, send_message);

	twitux_conf_get_bool (twitux_conf_get (),
						  TWITUX_PREFS_UI_SPELL,
						  &spell_checker);

	gtk_text_buffer_get_start_iter (buffer, &start);

	if (!spell_checker) {
		gtk_text_buffer_get_end_iter (buffer, &end);
		gtk_text_buffer_remove_tag_by_name (buffer, "misspelled", &start, &end);
		return;
	}

	if (!twitux_spell_supported ()) {
		return;
	}

	while (TRUE) {
		gboolean correct = FALSE;

		/* if at start */
		if (gtk_text_iter_is_start (&start)) {
			end = start;

			if (!gtk_text_iter_forward_word_end (&end)) {
				/* no whole word yet */
				break;
			}
		} else {
			if (!gtk_text_iter_forward_word_end (&end)) {
				/* must be the end of the buffer */
				break;
			}

			start = end;
			gtk_text_iter_backward_word_start (&start);
		}

		str = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

		/* spell check string */
		correct = twitux_spell_check (str);

		if (!correct) {
			gtk_text_buffer_apply_tag_by_name (buffer, "misspelled",
											   &start, &end);
		} else {
			gtk_text_buffer_remove_tag_by_name (buffer, "misspelled",
												&start, &end);
		}

		g_free (str);

		/* set the start iter to the end iters position */
		start = end;
	}
}

static void
message_text_populate_popup_cb (GtkTextView        *view,
								GtkMenu            *menu,
								TwituxSendMessage  *send_message)
{
	GtkTextBuffer      *buffer;
	GtkTextTagTable    *table;
	GtkTextTag         *tag;
	gint                x,y;
	GtkTextIter         iter, start, end;
	GtkWidget          *item;
	gchar              *str = NULL;
	TwituxMessageSpell *message_spell;

	/* Add the spell check menu item */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (send_message->textview));
	table = gtk_text_buffer_get_tag_table (buffer);

	tag = gtk_text_tag_table_lookup (table, "misspelled");

	gtk_widget_get_pointer (GTK_WIDGET (view), &x, &y);

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (view),
										   GTK_TEXT_WINDOW_WIDGET,
										   x, y,
										   &x, &y);

	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (view), &iter, x, y);

	start = end = iter;

	if (gtk_text_iter_backward_to_tag_toggle (&start, tag) &&
		gtk_text_iter_forward_to_tag_toggle (&end, tag)) {
		str = gtk_text_buffer_get_text (buffer,
										&start, &end, FALSE);
	}

	if (G_STR_EMPTY (str)) {
		return;
	}

	message_spell = message_spell_new (send_message->textview, str, start, end);

	g_object_set_data_full (G_OBJECT (menu),
							"message_spell", message_spell,
							(GDestroyNotify) message_spell_free);

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new_with_mnemonic (_("_Check Word Spelling..."));
	g_signal_connect (item,
					  "activate",
					  G_CALLBACK (message_text_check_word_spelling_cb),
					  message_spell);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
}

static void
message_text_check_word_spelling_cb (GtkMenuItem        *menuitem,
									 TwituxMessageSpell *chat_spell)
{
	twitux_spell_dialog_show (chat_spell->textview,
							  chat_spell->start,
							  chat_spell->end,
							  chat_spell->word);
}

static TwituxMessageSpell *
message_spell_new (GtkWidget          *textview,
				   const gchar        *word,
				   GtkTextIter         start,
				   GtkTextIter         end)
{
	TwituxMessageSpell *message_spell;

	message_spell = g_new0 (TwituxMessageSpell, 1);

	message_spell->textview = textview;
	message_spell->word = g_strdup (word);
	message_spell->start = start;
	message_spell->end = end;

	return message_spell;
}

static void
message_spell_free (TwituxMessageSpell *message_spell)
{
	g_free (message_spell->word);
	g_free (message_spell);
}

static void
message_response_cb (GtkWidget          *widget,
					 gint                response,
					 TwituxSendMessage  *send_message)
{
	if (response == GTK_RESPONSE_OK) {	  
		GtkTextBuffer  *buffer;
		GtkTextIter     start_iter;
		GtkTextIter     end_iter;

		twitux_debug (DEBUG_DOMAIN, "Posting message to Twitter");

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (send_message->textview));
		gtk_text_buffer_get_start_iter (buffer, &start_iter);
		gtk_text_buffer_get_end_iter (buffer, &end_iter);

		if (!gtk_text_iter_equal (&start_iter, &end_iter)) {
				gchar          *text;
				gchar          *good_msg;

				text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, TRUE);
				good_msg = url_encode_message (text);

				twitux_network_post_status (good_msg);

				g_free (text);
				g_free (good_msg);
		}
	}
	gtk_widget_destroy (widget);
}

static void
message_destroy_cb (GtkWidget         *widget,
					TwituxSendMessage *send_message)
{
	g_free (send_message);
}

void
twitux_send_message_dialog_show (GtkWindow *parent)
{
	static TwituxSendMessage *send_message;
	GladeXML                 *glade;
	GtkTextBuffer            *buffer;
	const gchar              *standard_msg;
	gchar                    *character_count;

	if (send_message) {
		gtk_window_present (GTK_WINDOW (send_message->dialog));
		return;
	}

	twitux_debug (DEBUG_DOMAIN, "Creating send message dialog...");

	send_message = g_new0 (TwituxSendMessage, 1);

	glade = twitux_glade_get_file ("main.glade",
								   "send_message_dialog",
								   NULL,
								   "send_message_dialog", &send_message->dialog,
								   "send_message_textview", &send_message->textview,
								   "char_label", &send_message->label,
								   "send_button", &send_message->send_button,
								   NULL);

	twitux_glade_connect (glade,
						  send_message,
						  "send_message_dialog", "destroy", message_destroy_cb,
						  "send_message_dialog", "response", message_response_cb,
						  "send_message_textview", "populate_popup", message_text_populate_popup_cb,
						  NULL);

	g_object_unref (glade);

	/* Set the label */
	standard_msg = _("Characters Available");
	character_count =
		g_markup_printf_escaped ("<span size=\"small\">%s: <span foreground=\"black\">%i</span></span>",
								 standard_msg, MAX_CHARACTER_COUNT);
	gtk_label_set_markup (GTK_LABEL (send_message->label), character_count);
	g_free (character_count);

	/* Connect the signal to the textview */
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (send_message->textview));
	g_signal_connect (buffer,
					  "changed",
					  G_CALLBACK (message_text_buffer_changed_cb),
					  send_message);

	/* Create misspelt words identification tag */
	gtk_text_buffer_create_tag (buffer,
								"misspelled",
								"underline", PANGO_UNDERLINE_ERROR,
								NULL);

	g_object_add_weak_pointer (G_OBJECT (send_message->dialog),
							   (gpointer) &send_message);

	gtk_window_set_transient_for (GTK_WINDOW (send_message->dialog), parent);

	gtk_widget_show (send_message->dialog);
}

void
twitux_message_correct_word (GtkWidget   *textview,
							 GtkTextIter  start,
							 GtkTextIter  end,
							 const gchar *new_word)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (textview != NULL);
	g_return_if_fail (new_word != NULL);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_insert (buffer, &start,
							new_word,
							-1);
}
