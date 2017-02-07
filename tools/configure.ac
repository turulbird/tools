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

AC_ARG_ENABLE(fortis_hdbox,
	AS_HELP_STRING(--enable-fortis_hdbox, enable Fortis FS9000/9200 stuff),
	[case "${enableval}" in
		yes) FORTIS_HDBOX=fortis_hdbox ;;
	esac],[enable_fortis_hdbox=no])
AM_CONDITIONAL(ENABLE_FORTIS_HDBOX,test "$enable_fortis_hdbox" = "yes")
AC_SUBST(FORTIS_HDBOX)

AC_ARG_ENABLE(atevio7500,
	AS_HELP_STRING(--enable-atevio7500, enable Fortis HS8200 stuff),
	[case "${enableval}" in
		yes) ATEVIO7500=atevio7500 ;;
	esac],[enable_atevio7500=no])
AM_CONDITIONAL(ENABLE_ATEVIO7500,test "$enable_atevio7500" = "yes")
AC_SUBST(ATEVIO7500)

AC_ARG_ENABLE(octagon1008,
	AS_HELP_STRING(--enable-octagon1008, enable Fortis HS9510 stuff),
	[case "${enableval}" in
		yes) OCTAGON1008=octagon1008 ;;
	esac],[enable_octagon1008=no])
AM_CONDITIONAL(ENABLE_OCTAGON1008,test "$enable_octagon1008" = "yes")
AC_SUBST(OCTAGON1008)

AC_ARG_ENABLE(hs7110,
	AS_HELP_STRING(--enable-hs7110, enable Fortis HS7110 stuff),
	[case "${enableval}" in
		yes) HS7110=hs7110 ;;
	esac],[enable_hs7110=no])
AM_CONDITIONAL(ENABLE_HS7110,test "$enable_hs7110" = "yes")
AC_SUBST(HS7110)

AC_ARG_ENABLE(hs7420,
	AS_HELP_STRING(--enable-hs7420, enable Fortis HS7420 stuff),
	[case "${enableval}" in
		yes) HS7420=hs7420 ;;
	esac],[enable_hs7420=no])
AM_CONDITIONAL(ENABLE_HS7420,test "$enable_hs7420" = "yes")
AC_SUBST(HS7420)

AC_ARG_ENABLE(hs7810a,
	AS_HELP_STRING(--enable-hs7810a, enable Fortis HS7810A stuff),
	[case "${enableval}" in
		yes) HS7810A=hs7810a ;;
	esac],[enable_hs7810a=no])
AM_CONDITIONAL(ENABLE_HS7810A,test "$enable_hs7810a" = "yes")
AC_SUBST(HS7810A)

AC_ARG_ENABLE(hs7119,
	AS_HELP_STRING(--enable-hs7119, enable Fortis HS7119 stuff),
	[case "${enableval}" in
		yes) HS7119=hs7119 ;;
	esac],[enable_hs7119=no])
AM_CONDITIONAL(ENABLE_HS7119,test "$enable_hs7119" = "yes")
AC_SUBST(HS7119)

AC_ARG_ENABLE(hs7429,
	AS_HELP_STRING(--enable-hs7429, enable Fortis HS7429 stuff),
	[case "${enableval}" in
		yes) HS7429=hs7429 ;;
	esac],[enable_hs7429=no])
AM_CONDITIONAL(ENABLE_HS7429,test "$enable_hs7429" = "yes")
AC_SUBST(HS7429)

AC_ARG_ENABLE(hs7819,
	AS_HELP_STRING(--enable-hs7819, enable Fortis HS7819 stuff),
	[case "${enableval}" in
		yes) HS7819=hs7819 ;;
	esac],[enable_hs7819=no])
AM_CONDITIONAL(ENABLE_HS7819,test "$enable_hs7819" = "yes")
AC_SUBST(HS7819)

AC_ARG_ENABLE([tf7700],
	[AS_HELP_STRING(--enable-tf7700,    enable topfield tf7700 stuff)],
	[case "${enableval}" in
		yes) enable_tf7700=true ;;
		no) enable_tf7700=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-tf7700]) ;;
	esac])
AM_CONDITIONAL([ENABLE_TF7700], [test x$enable_tf7700 = xtrue])

AC_ARG_ENABLE(hl101,
	AS_HELP_STRING(--enable-hl101, enable spider box hl101 stuff),
	[case "${enableval}" in
		yes) HL101=hl101 ;;
	esac],[enable_hl101=no])
AM_CONDITIONAL(ENABLE_HL101,test "$enable_hl101" = "yes")
AC_SUBST(HL101)

AC_ARG_ENABLE(vip2_v1,
	AS_HELP_STRING(--enable-vip2_v1, enable edision argus vip2_v1 stuff),
	[case "${enableval}" in
		yes) VIP2_V1=vip2_v1 ;;
	esac],[enable_vip2_v1=no])
AM_CONDITIONAL(ENABLE_VIP2_V1,test "$enable_vip2_v1" = "yes")
AC_SUBST(VIP2_V1)

AC_ARG_ENABLE(vip1_v2,
	AS_HELP_STRING(--enable-vip1_v2, enable edision argus vip1_v2 stuff),
	[case "${enableval}" in
		yes) VIP1_V2=vip1_v2 ;;
	esac],[enable_vip1_v2=no])
AM_CONDITIONAL(ENABLE_VIP1_V2,test "$enable_vip1_v2" = "yes")
AC_SUBST(VIP1_V2)

AC_ARG_ENABLE(spark,
	AS_HELP_STRING(--enable-spark, enable spark stuff),
	[case "${enableval}" in
		yes) SPARK=spark ;;
	esac],[enable_spark=no])
AM_CONDITIONAL(ENABLE_SPARK,test "$enable_spark" = "yes")
AC_SUBST(SPARK)

AC_ARG_ENABLE(spark7162,
	AS_HELP_STRING(--enable-spark7162, enable spark7162 stuff),
	[case "${enableval}" in
		yes) SPARK7162=spark7162 ;;
	esac],[enable_spark7162=no])
AM_CONDITIONAL(ENABLE_SPARK7162,test "$enable_spark7162" = "yes")
AC_SUBST(SPARK7162)

AC_ARG_ENABLE(adb_box,
	AS_HELP_STRING(--enable-adb_box, enable adb_box stuff),
	[case "${enableval}" in
		yes) ADB_BOX=adb_box ;;
	esac],[enable_adb_box=no])
AM_CONDITIONAL(ENABLE_ADB_BOX,test "$enable_adb_box" = "yes")
AC_SUBST(ADB_BOX)

AC_ARG_ENABLE(ufs922,
	AS_HELP_STRING(--enable-ufs922, enable Kathrein ufs922 stuff),
	[case "${enableval}" in
		yes) UFS922=ufs922 ;;
	esac],[enable_ufs922=no])
AM_CONDITIONAL(ENABLE_UFS922,test "$enable_ufs922" = "yes")
AC_SUBST(UFS922)

AC_ARG_ENABLE(ufc960,
	AS_HELP_STRING(--enable-ufc960, enable Kathrein ufc960 stuff),
	[case "${enableval}" in
		yes) UFC960=ufc960 ;;
	esac],[enable_ufc960=no])
AM_CONDITIONAL(ENABLE_UFC960,test "$enable_ufc960" = "yes")
AC_SUBST(UFC960)

AC_ARG_ENABLE(ufs910,
	AS_HELP_STRING(--enable-ufs910, enable Kathrein ufs910 stuff),
	[case "${enableval}" in
		yes) UFS910=ufs910 ;;
	esac],[enable_ufs910=no])
AM_CONDITIONAL(ENABLE_UFS910,test "$enable_ufs910" = "yes")
AC_SUBST(UFS910)

AC_ARG_ENABLE(ufs912,
	AS_HELP_STRING(--enable-ufs912, enable Kathrein ufs912 stuff),
	[case "${enableval}" in
		yes) UFS912=ufs912 ;;
	esac],[enable_ufs912=no])
AM_CONDITIONAL(ENABLE_UFS912,test "$enable_ufs912" = "yes")
AC_SUBST(UFS912)

AC_ARG_ENABLE(ufs913,
	AS_HELP_STRING(--enable-ufs913, enable Kathrein ufs913 stuff),
	[case "${enableval}" in
		yes) UFS913=ufs913 ;;
	esac],[enable_ufs913=no])
AM_CONDITIONAL(ENABLE_UFS913,test "$enable_ufs913" = "yes")
AC_SUBST(UFS913)

AC_ARG_ENABLE(vitamin_hd5000,
	AS_HELP_STRING(--enable-vitamin_hd5000, enable vitamin_hd5000 stuff),
	[case "${enableval}" in
		yes) VITAMIN_HD5000=vitamin_hd5000 ;;
	esac],[enable_vitamin_hd5000=no])
AM_CONDITIONAL(ENABLE_VITAMIN_HD5000,test "$enable_vitamin_hd5000" = "yes")
AC_SUBST(VITAMIN_HD5000)

AC_ARG_ENABLE(sagemcom88,
	AS_HELP_STRING(--enable-sagemcom88, enable sagemcom88 stuff),
	[case "${enableval}" in
		yes) SAGEMCOM88=sagemcom88 ;;
	esac],[enable_sagemcom88=no])
AM_CONDITIONAL(ENABLE_SAGEMCOM88,test "$enable_sagemcom88" = "yes")
AC_SUBST(SAGEMCOM88)

AC_ARG_ENABLE(atemio520,
	AS_HELP_STRING(--enable-atemio520, enable atemio520 stuff),
	[case "${enableval}" in
		yes) ATEMIO520=atemio520 ;;
	esac],[enable_ATEMIO520=no])
AM_CONDITIONAL(ENABLE_ATEMIO520,test "$enable_atemio520" = "yes")
AC_SUBST(ATEMIO520)

AC_ARG_ENABLE(atemio530,
	AS_HELP_STRING(--enable-atemio530, enable atemio530 stuff),
	[case "${enableval}" in
		yes) ATEMIO530=atemio530 ;;
	esac],[enable_ATEMIO530=no])
AM_CONDITIONAL(ENABLE_ATEMIO530,test "$enable_atemio530" = "yes")
AC_SUBST(ATEMIO530)

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

AC_ARG_ENABLE(homecast5101,
	AS_HELP_STRING(--enable-homecast5101, enable homecast 5101 stuff),
	[case "${enableval}" in
		yes) HOMECAST5101=hs5101 ;;
	esac],[enable_homecast5101=no])
AM_CONDITIONAL(ENABLE_HOMECAST5101,test "$enable_homecast5101" = "yes")
AC_SUBST(HOMECAST5101)

AC_ARG_ENABLE(arivalink200,
	AS_HELP_STRING(--enable-arivalink200, enable arivalink200 stuff),
	[case "${enableval}" in
		yes) ARIVALINK200=arivalink200 ;;
	esac],[enable_arivalink200=no])
AM_CONDITIONAL(ENABLE_ARIVALINK200,test "$enable_arivalink200" = "yes")
AC_SUBST(ARIVALINK200)

AC_ARG_ENABLE(ipbox9900,
	AS_HELP_STRING(--enable-ipbox9900, enable ipbox 9900 stuff),
	[case "${enableval}" in
		yes) IPBOX9900=ipbox9900 ;;
	esac],[enable_ipbox9900=no])
AM_CONDITIONAL(ENABLE_IPBOX9900,test "$enable_ipbox9900" = "yes")
AC_SUBST(IPBOX9900)

AC_ARG_ENABLE(ipbox99,
	AS_HELP_STRING(--enable-ipbox99, enable ipbox 99 stuff),
	[case "${enableval}" in
		yes) IPBOX99=ipbox99 ;;
	esac],[enable_ipbox99=no])
AM_CONDITIONAL(ENABLE_IPBOX99,test "$enable_ipbox99" = "yes")
AC_SUBST(IPBOX99)

AC_ARG_ENABLE(ipbox55,
	AS_HELP_STRING(--enable-ipbox55, enable ipbox 55 stuff),
	[case "${enableval}" in
		yes) IPBOX55=ipbox55 ;;
	esac],[enable_ipbox55=no])
AM_CONDITIONAL(ENABLE_IPBOX55,test "$enable_ipbox55" = "yes")
AC_SUBST(IPBOX55)

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
