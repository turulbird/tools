AC_INIT([titan-tools],[1.0],[],[titan-tools])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([LT_INIT], [LT_INIT], [AC_PROG_LIBTOOL])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_CXX

PKG_CHECK_MODULES(FREETYPE, [freetype2])
PKG_CHECK_MODULES(TUXTXT, tuxbox-tuxtxt)

AC_ARG_WITH(fontdir,
        AC_HELP_STRING([--with-fontdir=PATH], [where to find the fonts]),
        [[FONTDIR=$withval]],
        [[FONTDIR=no]]
)
if test x"$with_fontdir" != xno ; then
        AC_DEFINE_UNQUOTED(FONTDIR, "$withval", [where to find the fonts])
fi
AC_SUBST(FONTDIR)

AC_OUTPUT([
Makefile
sleepms/Makefile
tuxtxt/Makefile
])
