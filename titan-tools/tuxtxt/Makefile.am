bin_PROGRAMS = tuxtxt

tuxtxt_SOURCES = tuxtxt.c

AM_CFLAGS = -Wall -DHAVE_DVB_API_VERSION=5 -I$(srcdir)/../../../tufsbox/cdkroot/usr/include/freetype2

tuxtxt_LDADD = -ltuxtxt -ltuxtxt32bpp -lpthread -lfreetype

