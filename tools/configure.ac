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

AC_ARG_ENABLE(gstreamer,
	AS_HELP_STRING(--enable-gstreamer,    enable gstreamer support),
	[case "${enableval}" in
		yes) GSTREAMER=gstreamer ;;
	esac],[enable_gstreamer=no])
AM_CONDITIONAL(ENABLE_GSTREAMER,test "$enable_gstreamer" = "yes")
AC_SUBST(GSTREAMER)

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

AC_ARG_ENABLE([tf7700],
	[AS_HELP_STRING(--enable-tf7700,    enable topfield tf7700 stuff)],
	[case "${enableval}" in
		yes) enable_tf7700=true ;;
		no) enable_tf7700=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-tf7700]) ;;
	esac])
AM_CONDITIONAL([ENABLE_TF7700], [test x$enable_tf7700 = xtrue])

AC_ARG_ENABLE([cuberevo],
	[AS_HELP_STRING(--enable-cuberevo,    enable cuberevo stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo=true ;;
		no) enable_cuberevo=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO], [test x$enable_cuberevo = xtrue])

AC_ARG_ENABLE([cuberevo-mini],
	[AS_HELP_STRING(--enable-cuberevo-mini,    enable cuberevo-mini stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_mini=true ;;
		no) enable_cuberevo_mini=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-mini]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_MINI], [test x$enable_cuberevo_mini = xtrue])

AC_ARG_ENABLE([cuberevo-mini2],
	[AS_HELP_STRING(--enable-cuberevo-mini2,    enable cuberevo-mini2 stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_mini2=true ;;
		no) enable_cuberevo_mini2=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-mini2]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_MINI2], [test x$enable_cuberevo_mini2 = xtrue])

AC_ARG_ENABLE([cuberevo-mini-fta],
	[AS_HELP_STRING(--enable-cuberevo-mini-fta, enable cuberevo-mini-fta stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_mini_fta=true ;;
		no) enable_cuberevo_mini_fta=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-mini-fta]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_MINI_FTA], [test x$enable_cuberevo_mini_fta = xtrue])

AC_ARG_ENABLE([cuberevo-250hd],
	[AS_HELP_STRING(--enable-cuberevo-250hd,    enable cuberevo-250hd stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_250hd=true ;;
		no) enable_cuberevo_250hd=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-250hd]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_250HD], [test x$enable_cuberevo_250hd = xtrue])

AC_ARG_ENABLE([cuberevo-2000hd],
	[AS_HELP_STRING(--enable-cuberevo-2000hd,   enable cuberevo-2000hd stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_2000hd=true ;;
		no) enable_cuberevo_2000hd=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-2000hd]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_2000HD], [test x$enable_cuberevo_2000hd = xtrue])

AC_ARG_ENABLE([cuberevo-3000hd],
	[AS_HELP_STRING(--enable-cuberevo-3000hd,   enable cuberevo-3000hd stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_3000hd=true ;;
		no) enable_cuberevo_3000hd=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-3000]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_3000HD], [test x$enable_cuberevo_3000hd = xtrue])

AC_ARG_ENABLE([cuberevo-9500hd],
	[AS_HELP_STRING(--enable-cuberevo-9500hd,   enable cuberevo-9500hd stuff)],
	[case "${enableval}" in
		yes) enable_cuberevo_9500hd=true ;;
		no) enable_cuberevo_9500hd=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cuberevo-9500]) ;;
	esac])
AM_CONDITIONAL([ENABLE_CUBEREVO_9500HD], [test x$enable_cuberevo_9500hd = xtrue])


AC_MSG_RESULT([
summary:
----------------------------------------
Boxtype          : $BOXTYPE
Multicom324      : $enable_multicom324
Multicom406      : $enable_multicom406
libeplayer3      : $enable_eplayer3
gstreamer        : $enable_gstreamer
])

AC_OUTPUT([
Makefile
aio-grab/Makefile
devinit/Makefile
eplayer3/Makefile
evremote2/Makefile
fbshot/Makefile
fp_control/Makefile
gitVCInfo/Makefile
hd-idle/Makefile
hotplug/Makefile
ipbox_eeprom/Makefile
libeplayer3/Makefile
libmme_host/Makefile
libmme_image/Makefile
showiframe/Makefile
stfbcontrol/Makefile
streamproxy/Makefile
tfd2mtd/Makefile
tffpctl/Makefile
ustslave/Makefile
vfdctl/Makefile
spf_tool/Makefile
wait4button/Makefile
])
