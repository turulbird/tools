ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = \
	@FREETYPE_CFLAGS@ \
	-I$(top_builddir) \
	-I$(top_srcdir)/include

noinst_PROGRAMS = tuxcom

tuxcom_SOURCES = tuxcom.c

tuxcom_LDADD = \
	@FREETYPE_LIBS@

install-exec-local:
	install -d $(DESTDIR)/var/tuxbox/plugins
	install -m 0755 tuxcom $(DESTDIR)/var/tuxbox/plugins/tuxcom.so
	install -m 0644 $(srcdir)/tuxcom.cfg $(DESTDIR)/var/tuxbox/plugins
	install -m 0644 $(srcdir)/tuxcom_hint.png $(DESTDIR)/var/tuxbox/plugins
