AC_INIT([infobox],[1.8],[],[infobox])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([LT_INIT], [LT_INIT], [AC_PROG_LIBTOOL])

AC_CONFIG_MACRO_DIR([m4])
AC_CHECK_LIB(freetype, freetype_create, [LIBS="$LIBS -lfreetype"])
AC_CONFIG_HEADERS([config.h])

AC_OUTPUT([
Makefile
])
