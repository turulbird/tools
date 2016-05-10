AC_INIT([tuxbox-tools], [0.0.1])
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

TUXBOX_BOXTYPE

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_ARG_ENABLE([multicom324],
	[AS_HELP_STRING(--enable-multicom324,    enable multicom324 stuff)],
	[case "${enableval}" in
		yes) enable_multicom324=true ;;
	esac],[enable_multicom324=no])
AM_CONDITIONAL([ENABLE_MULTICOM324], [test x$enable_multicom324 = xtrue])
AC_SUBST(MULTICOM324)

AC_ARG_ENABLE([multicom406],
	[AS_HELP_STRING(--enable-multicom406,    enable multicom406 stuff)],
	[case "${enableval}" in 
		yes) enable_multicom406=true ;;
	esac],[enable_multicom406=no])
AM_CONDITIONAL([ENABLE_MULTICOM406], [test x$enable_multicom406 = xtrue])
AC_SUBST(MULTICOM406)

AC_ARG_ENABLE(eplayer3,
	AS_HELP_STRING(--enable-eplayer3,    enable eplayer3 support),
	[case "${enableval}" in
		yes) EPLAYER3=eplayer3 ;;
	esac],[enable_eplayer3=no])
AM_CONDITIONAL(ENABLE_EPLAYER3,test "$enable_eplayer3" = "yes")
AC_SUBST(EPLAYER3)

AC_ARG_ENABLE(spf,
	AS_HELP_STRING(--enable-spf,    enable spf support),
	[case "${enableval}" in
		yes) SPF=spf ;;
	esac],[enable_spf=no])
AM_CONDITIONAL(ENABLE_SPF,test "$enable_spf" = "yes")
AC_SUBST(SPF)

AC_MSG_RESULT([
summary:
----------------------------------------
Boxtype          : $BOXTYPE
Multicom324      : $enable_multicom324
Multicom406      : $enable_multicom406
libeplayer3      : $enable_eplayer3
])

AC_OUTPUT([
Makefile
aio-grab/Makefile
devinit/Makefile
evremote2/Makefile
wait4button/Makefile
libeplayer3/Makefile
fp_control/Makefile
fbshot/Makefile
showiframe/Makefile
stfbcontrol/Makefile
streamproxy/Makefile
ustslave/Makefile
tffpctl/Makefile
tfd2mtd/Makefile
gitVCInfo/Makefile
hotplug/Makefile
vfdctl/Makefile
hd-idle/Makefile
libmme_host/Makefile
libmme_image/Makefile
ipbox_eeprom/Makefile
spf_tool/Makefile
])
