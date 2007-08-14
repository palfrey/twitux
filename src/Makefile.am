## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = twitux

twitux_SOURCES = \
	main.c main.h \
	callbacks.c callbacks.h \
	common.c common.h \
	conf.c conf.h \
	gcommon.c gcommon.h \
	gui.c gui.h \
	network.c network.h \
	twitter.c twitter.h

twitux_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

