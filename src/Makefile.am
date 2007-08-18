AM_CPPFLAGS=						\
	-I$(top_srcdir)                          	\
	-DPREFIX="\"$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"              	\
	-DLIBDIR=\""$(libdir)"\"			\
	$(TWITUX_CFLAGS)

bin_PROGRAMS = twitux

twitux_SOURCES = 					\
	main.c 				main.h 		\
	callbacks.c			callbacks.h	\
	common.c			common.h	\
	gcommon.c			gcommon.h	\
	gui.c				gui.h		\
	network.c			network.h	\
	twitter.c			twitter.h	\
	twitux-preferences.h

twitux_LDADD=						\
	$(top_builddir)/libtwitux/libtwitux.la		\
	$(TWITUX_LIBS)

