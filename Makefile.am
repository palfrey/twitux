SUBDIRS = pixmaps src po

desktop_in_files=twitux.desktop.in
desktop_files=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

Applicationsdir = $(datadir)/applications
Applications_DATA = $(desktop_files)

INTLTOOL = 			\
	intltool-extract.in 	\
	intltool-merge.in 	\
	intltool-update.in

EXTRA_DIST = 			\
	ChangeLog               \
	README                  \
	AUTHORS			\
	NEWS			\
	$(desktop_in_files)	\
	$(desktop_files)	\
	$(INTLTOOL)

DISTCLEANFILES = 		\
	intltool-extract	\
	intltool-merge		\
	intltool-update		\
	$(desktop_files)
