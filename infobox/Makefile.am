ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS  = -Wall
AM_CFLAGS += -std=c99
AM_CFLAGS += -I../../tufsbox/cdkroot/usr/include/freetype2

bin_PROGRAMS = infobox

infobox_SOURCES = readpng.c infobox.c
infobox_LDADD = -lm -lfreetype -ljpeg -lpng -lz


