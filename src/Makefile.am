AM_CPPFLAGS=						\
	-I$(top_srcdir)                          	\
	-DPREFIX="\"$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"              	\
	-DLIBDIR=\""$(libdir)"\"			\
	$(TWITUX_CFLAGS)

bin_PROGRAMS = twitux

if HAVE_DBUS
dbus_sources = twitux-dbus.c twitux-dbus.h
endif

twitux_SOURCES = 							\
	$(dbus_sources)							\
	twitux.h							\
	twitux-main.c							\
	twitux-about.c			twitux-about.h			\
	twitux-account-dialog.c 	twitux-account-dialog.h		\
	twitux-app.c			twitux-app.h			\
	twitux-avatar.c			twitux-avatar.h			\
	twitux-geometry.c		twitux-geometry.h		\
	twitux-glade.c			twitux-glade.h			\
	twitux-network.c		twitux-network.h		\
	twitux-parser.c			twitux-parser.h			\
	twitux-send-message-dialog.c	twitux-send-message-dialog.h	\
	twitux-spell.c			twitux-spell.h			\
	twitux-spell-dialog.c		twitux-spell-dialog.h		\
	twitux-ui-utils.c		twitux-ui-utils.h		\
	twitux-preferences.c		twitux-preferences.h

twitux_LDADD=						\
	$(top_builddir)/libtwitux/libtwitux.la		\
	$(TWITUX_LIBS)

