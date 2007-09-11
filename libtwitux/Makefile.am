AM_CPPFLAGS =                                           \
	-I.                                             \
	-I$(top_srcdir)                                 \
	-DDATADIR=\""$(datadir)"\"              	\
	-DLOCALEDIR=\""$(datadir)/locale"\"		\
	$(LIBTWITUX_CFLAGS)

noinst_LTLIBRARIES = libtwitux.la

libtwitux_la_SOURCES =             			\
	twitux-avatar.c		twitux-avatar.h		\
	twitux-conf.c		twitux-conf.h		\
	twitux-debug.c		twitux-debug.h		\
	twitux-paths.c 		twitux-paths.h

libtwitux_la_LIBADD =              			\
	$(LIBTWITUX_LIBS)

libtwituxincludedir = $(includedir)/twitux/


