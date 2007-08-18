desktopdir 	 = $(datadir)/applications
desktop_in_files = twitux.desktop.in.in
desktop_DATA 	 = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemasdir	 = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = twitux.schemas.in
schemas_DATA 	 = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = 			\
	twitux-loading.png	\
	twitux.png		\
	twitux-side.png		\
	twitux-top.png
	
EXTRA_DIST =			\
	$(pixmap_DATA)		\
	$(desktop_in_files)	\
	$(schemas_in_files)
	
DISTCLEANFILES =		\
	$(desktop_DATA)		\
	$(schemas_DATA)
	
if GCONF_SCHEMAS_INSTALL
install-data-local:
	        if test -z "$(DESTDIR)" ; then \
	                for p in $(schemas_DATA) ; do \
	                        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p ; \
	                done \
	        fi
else
install-data-local:
endif
	
