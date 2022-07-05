AC_DEFUN([TUXBOX_APPS], [
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_GNU_SOURCE

AC_ARG_WITH(target,
	AS_HELP_STRING([--with-target=TARGET], [target for compilation [[native,cdk]]]),
	[TARGET="$withval"],
	[TARGET="native"])

AC_ARG_WITH(targetprefix,
	AS_HELP_STRING([--with-targetprefix=PATH], [prefix relative to target root (only applicable in cdk mode)]),
	[TARGET_PREFIX="$withval"],
	[TARGET_PREFIX=""])

AC_ARG_WITH(debug,
	AS_HELP_STRING([--without-debug], [disable debugging code @<:@default=no@:>@]),
	[DEBUG="$withval"],
	[DEBUG="yes"])

if test "$DEBUG" = "yes"; then
	DEBUG_CFLAGS="-g3 -ggdb"
	AC_DEFINE(DEBUG, 1, [enable debugging code])
fi

AC_MSG_CHECKING(target)

if test "$TARGET" = "native"; then
	AC_MSG_RESULT(native)

	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
		CXXFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
	fi
	if test "$prefix" = "NONE"; then
		prefix=/usr/local
	fi
	TARGET_PREFIX=$prefix
	if test "$exec_prefix" = "NONE"; then
		exec_prefix=$prefix
	fi
	targetprefix=$prefix
elif test "$TARGET" = "cdk"; then
	AC_MSG_RESULT(cdk)

	if test "$CC" = "" -a "$CXX" = ""; then
		AC_MSG_ERROR([you need to specify variables CC or CXX in cdk])
	fi
	if test "$CFLAGS" = "" -o "$CXXFLAGS" = ""; then
		AC_MSG_ERROR([you need to specify variables CFLAGS and CXXFLAGS in cdk])
	fi
	if test "$prefix" = "NONE"; then
		AC_MSG_ERROR([invalid prefix, you need to specify one in cdk mode])
	fi
	if test "$TARGET_PREFIX" != "NONE"; then
		AC_DEFINE_UNQUOTED(TARGET_PREFIX, "$TARGET_PREFIX", [The targets prefix])
	fi
	if test "$TARGET_PREFIX" = "NONE"; then
		AC_MSG_ERROR([invalid targetprefix, you need to specify one in cdk mode])
		TARGET_PREFIX=""
	fi
else
	AC_MSG_RESULT(none)
	AC_MSG_ERROR([invalid target $TARGET, choose on from native,cdk]);
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_SYS_LARGEFILE

])

dnl expand nested ${foo}/bar
AC_DEFUN([TUXBOX_EXPAND_VARIABLE], [
	__$1="$2"
	for __CNT in false false false false true; do dnl max 5 levels of indirection
		$1=`eval echo "$__$1"`
		echo ${$1} | grep -q '\$' || break # 'grep -q' is POSIX, exit if no $ in variable
		__$1="${$1}"
	done
	$__CNT && AC_MSG_ERROR([can't expand variable $1=$2]) dnl bail out if we did not expand
])

AC_DEFUN([TUXBOX_APPS_DIRECTORY_ONE], [
AC_ARG_WITH($1, AS_HELP_STRING([$6], [$7 [[PREFIX$4$5]]], [32], [79]), [
	_$2=$withval
	if test "$TARGET" = "cdk"; then
		$2=`eval echo "$TARGET_PREFIX$withval"` # no indirection possible IMNSHO
	else
		$2=$withval
	fi
	TARGET_$2=${$2}
], [
	# RFC 1925: "you can always add another level of indirection..."
	TUXBOX_EXPAND_VARIABLE($2,"${$3}$5")
	if test "$TARGET" = "cdk"; then
		TUXBOX_EXPAND_VARIABLE(_$2,"${target$3}$5")
	else
		_$2=${$2}
	fi
	TARGET_$2=$_$2
])
dnl AC_SUBST($2)
AC_DEFINE_UNQUOTED($2,"$_$2",$7)
AC_SUBST(TARGET_$2)
])

AC_DEFUN([TUXBOX_APPS_DIRECTORY], [
AC_REQUIRE([TUXBOX_APPS])

if test "$TARGET" = "cdk"; then
	datadir="\${prefix}/share"
	sysconfdir="\${prefix}/etc"
	localstatedir="\${prefix}/var"
	libdir="\${prefix}/lib"
	targetdatadir="\${TARGET_PREFIX}/share"
	targetsysconfdir="\${TARGET_PREFIX}/etc"
	targetlocalstatedir="\${TARGET_PREFIX}/var"
	targetlibdir="\${TARGET_PREFIX}/lib"
fi

TUXBOX_APPS_DIRECTORY_ONE(configdir, CONFIGDIR, localstatedir, /var, /tuxbox/config,
	[--with-configdir=PATH], [where to find config files])

TUXBOX_APPS_DIRECTORY_ONE(datadir, DATADIR, datadir, /share, /tuxbox,
	[--with-datadir=PATH], [where to find data files])

TUXBOX_APPS_DIRECTORY_ONE(fontdir, FONTDIR, datadir, /share, /fonts,
	[--with-fontdir=PATH], [where to find fonts])

TUXBOX_APPS_DIRECTORY_ONE(gamesdir, GAMESDIR, localstatedir, /var, /tuxbox/games,
	[--with-gamesdir=PATH], [where to find games])

TUXBOX_APPS_DIRECTORY_ONE(libdir, LIBDIR, libdir, /lib, /tuxbox,
	[--with-libdir=PATH], [where to find internal libs])

TUXBOX_APPS_DIRECTORY_ONE(plugindir, PLUGINDIR, localstatedir, /var, /tuxbox/plugins,
	[--with-plugindir=PATH], [where to find plugins in /var])

TUXBOX_APPS_DIRECTORY_ONE(themesdir, THEMESDIR, datadir, /share, /tuxbox/neutrino/themes,
	[--with-themesdir=PATH], [where to find themes])
])

dnl automake <= 1.6 needs this specifications
AC_SUBST(CONFIGDIR)
AC_SUBST(DATADIR)
AC_SUBST(FONTDIR)
AC_SUBST(GAMESDIR)
AC_SUBST(LIBDIR)
AC_SUBST(PLUGINDIR)
AC_SUBST(THEMESDIR)
dnl end workaround

AC_DEFUN([TUXBOX_BOXTYPE], [
AC_ARG_WITH(boxtype,
	AS_HELP_STRING([--with-boxtype], [valid values: spark, generic, armbox, duckbox, spark7162, mipsbox]),
	[case "${withval}" in
		generic|armbox)
			BOXTYPE="$withval"
		;;
		spark|spark7162)
			BOXTYPE="spark"
			BOXMODEL="$withval"
		;;
		ufs*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		atevio*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		fortis*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		octagon*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		hs8200)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		hs7*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		cuberevo*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		ipbox*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		adb*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		tf*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		vitamin*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		sagemcom88)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		arivalink200)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		pace7241)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		atemio*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		opt*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
		;;
		hd51|multibox|multiboxse|hd60|hd61|bre2ze4k|vusolo4k|vuduo4k|vuduo4kse|vuultimo4k|vuzero4k|vuuno4kse|vuuno4k|h7|osmini4k|osmio4k|osmio4kplus)
			BOXTYPE="armbox"
			BOXMODEL="$withval"
		;;
		vuduo|vuduo2|dm8000|gb800se|osnino|osninoplus|osninopro)
			BOXTYPE="mipsbox"
			BOXMODEL="$withval"
		;;
		*)
			AC_MSG_ERROR([bad value $withval for --with-boxtype])
		;;
	esac],
	[BOXTYPE="generic"])

AC_ARG_WITH(boxmodel,
	AS_HELP_STRING([--with-boxmodel], [valid for generic: raspi])
AS_HELP_STRING([], [valid for duckbox: adb_box, adb_2850, ufs910, ufs912, ufs913, ufs922, ufc960, fs9000, hs9510, hs7110, hs7119, hs7420, hs7429, hs7810a, hs7819, hs8200, cuberevo, cuberevo_mini, cuberevo_mini2, cuberevo_mini_fta, cuberevo_250hd, cuberevo_2000hd, cuberevo_3000hd, cuberevo_9500hd, atemio520, ipbox9900, ipbox99, ipbox55, vip1_v1, vip1_v2, vip2, hl101, sagemcom88, arivalink200, pace7241, tf7700, vitamin_hd5000, opt9600, opt9600mini])
AS_HELP_STRING([], [valid for spark: spark, spark7162])
AS_HELP_STRING([], [valid for armbox: bre2ze4k, hd51, multibox, multiboxse, hd60, hd61, vusolo4k, vuduo4k, vuduo4kse, vuultimo4k, vuzero4k, vuuno4kse, vuuno4k, h7, osmini4k, osmio4k, osmio4kplus])
AS_HELP_STRING([], [valid for mipsbox: vuduo, vuduo2, dm8000, gb800se, osnino, osninoplus, osninopro]),
	[case "${withval}" in
		adb_box|adb_2850|ufs910|ufs912|ufs913|ufs922|ufc960|fs9000|hs9510|hs7110|hs7119|hs7420|hs7429|hs7810a|hs7819|hs8200|cuberevo|cuberevo_mini|cuberevo_mini2|cuberevo_mini_fta|cuberevo_250hd|cuberevo_2000hd|cuberevo_3000hd|cuberevo_9500hd|atemio520|ipbox9900|ipbox99|ipbox55|vip1_v1|vip1_v2|vip2|hl101|sagemcom88|arivalink200|pace7241|tf7700|vitamin_hd5000|opt9600|opt9600mini)
			if test "$BOXTYPE" = "duckbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
		;;
		spark|spark7162)
			if test "$BOXTYPE" = "spark"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
		;;
		hd51|multibox|multiboxse|hd60|hd61|bre2ze4k|vusolo4k|vuduo4k|vuduo4kse|vuultimo4k|vuzero4k|vuuno4kse|vuuno4k|h7|osmini4k|osmio4k|osmio4kplus)
			if test "$BOXTYPE" = "armbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
		;;
		vuduo|vuduo2|dm8000|gb800se|osnino|osninoplus|osninopro)
			if test "$BOXTYPE" = "mipsbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
		;;
		raspi)
			if test "$BOXTYPE" = "generic"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
		;;
		*)
			AC_MSG_ERROR([unsupported value $withval for --with-boxmodel])
		;;
	esac])

AC_SUBST(BOXTYPE)
AC_SUBST(BOXMODEL)

AM_CONDITIONAL(BOXTYPE_AZBOX, test "$BOXTYPE" = "azbox")
AM_CONDITIONAL(BOXTYPE_TRIPLE, test "$BOXTYPE" = "tripledragon")
AM_CONDITIONAL(BOXTYPE_SPARK, test "$BOXTYPE" = "spark")
AM_CONDITIONAL(BOXTYPE_GENERIC, test "$BOXTYPE" = "generic")
AM_CONDITIONAL(BOXTYPE_DUCKBOX, test "$BOXTYPE" = "duckbox")
AM_CONDITIONAL(BOXTYPE_ARMBOX, test "$BOXTYPE" = "armbox")
AM_CONDITIONAL(BOXTYPE_MIPSBOX, test "$BOXTYPE" = "mipsbox")

AM_CONDITIONAL(BOXMODEL_UFS910, test "$BOXMODEL" = "ufs910")
AM_CONDITIONAL(BOXMODEL_UFS912, test "$BOXMODEL" = "ufs912")
AM_CONDITIONAL(BOXMODEL_UFS913, test "$BOXMODEL" = "ufs913")
AM_CONDITIONAL(BOXMODEL_UFS922, test "$BOXMODEL" = "ufs922")
AM_CONDITIONAL(BOXMODEL_UFC960, test "$BOXMODEL" = "ufc960")
AM_CONDITIONAL(BOXMODEL_SPARK, test "$BOXMODEL" = "spark")
AM_CONDITIONAL(BOXMODEL_SPARK7162, test "$BOXMODEL" = "spark7162")
AM_CONDITIONAL(BOXMODEL_HS8200, test "$BOXMODEL" = "hs8200")
AM_CONDITIONAL(BOXMODEL_FS9000, test "$BOXMODEL" = "fs9000")
AM_CONDITIONAL(BOXMODEL_HS9510, test "$BOXMODEL" = "hs9510")
AM_CONDITIONAL(BOXMODEL_HS7110, test "$BOXMODEL" = "hs7110")
AM_CONDITIONAL(BOXMODEL_HS7119, test "$BOXMODEL" = "hs7119")
AM_CONDITIONAL(BOXMODEL_HS7420, test "$BOXMODEL" = "hs7420")
AM_CONDITIONAL(BOXMODEL_HS7429, test "$BOXMODEL" = "hs7429")
AM_CONDITIONAL(BOXMODEL_HS7810A, test "$BOXMODEL" = "hs7810a")
AM_CONDITIONAL(BOXMODEL_HS7819, test "$BOXMODEL" = "hs7819")

AM_CONDITIONAL(BOXMODEL_CUBEREVO, test "$BOXMODEL" = "cuberevo")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_MINI, test "$BOXMODEL" = "cuberevo_mini")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_MINI2, test "$BOXMODEL" = "cuberevo_mini2")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_250HD, test "$BOXMODEL" = "cuberevo_250hd")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_2000HD, test "$BOXMODEL" = "cuberevo_2000hd")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_3000HD, test "$BOXMODEL" = "cuberevo_3000hd")
AM_CONDITIONAL(BOXMODEL_IPBOX9900, test "$BOXMODEL" = "ipbox9900")
AM_CONDITIONAL(BOXMODEL_IPBOX99, test "$BOXMODEL" = "ipbox99")
AM_CONDITIONAL(BOXMODEL_IPBOX55, test "$BOXMODEL" = "ipbox55")
AM_CONDITIONAL(BOXMODEL_TF7700, test "$BOXMODEL" = "tf7700")
AM_CONDITIONAL(BOXMODEL_ATEMIO520, test "$BOXMODEL" = "atemio520")

AM_CONDITIONAL(BOXMODEL_VIP1_V1, test "$BOXMODEL" = "vip1_v1")
AM_CONDITIONAL(BOXMODEL_VIP1_V2, test "$BOXMODEL" = "vip1_v2")
AM_CONDITIONAL(BOXMODEL_VIP2, test "$BOXMODEL" = "vip2")
AM_CONDITIONAL(BOXMODEL_HL101, test "$BOXMODEL" = "hl101")
AM_CONDITIONAL(BOXMODEL_ADB_BOX, test "$BOXMODEL" = "adb_box")
AM_CONDITIONAL(BOXMODEL_ADB_2850, test "$BOXMODEL" = "adb_2850")
AM_CONDITIONAL(BOXMODEL_VITAMIN_HD5000, test "$BOXMODEL" = "vitamin_hd5000")
AM_CONDITIONAL(BOXMODEL_SAGEMCOM88, test "$BOXMODEL" = "sagemcom88")
AM_CONDITIONAL(BOXMODEL_ARRIVALINK200, test "$BOXMODEL" = "arivalink200")
AM_CONDITIONAL(BOXMODEL_PACE7241, test "$BOXMODEL" = "pace7241")
AM_CONDITIONAL(BOXMODEL_OPT9600, test "$BOXMODEL" = "opt9600")
AM_CONDITIONAL(BOXMODEL_OPT9600MINI, test "$BOXMODEL" = "opt9600mini")
AM_CONDITIONAL(BOXMODEL_OPT9600PRIMA, test "$BOXMODEL" = "opt9600prima")

AM_CONDITIONAL(BOXMODEL_HD51, test "$BOXMODEL" = "hd51")
AM_CONDITIONAL(BOXMODEL_MULTIBOX, test "$BOXMODEL" = "multibox")
AM_CONDITIONAL(BOXMODEL_MULTIBOXSE, test "$BOXMODEL" = "multiboxse")
AM_CONDITIONAL(BOXMODEL_HD60, test "$BOXMODEL" = "hd60")
AM_CONDITIONAL(BOXMODEL_HD61, test "$BOXMODEL" = "hd61")
AM_CONDITIONAL(BOXMODEL_BRE2ZE4K, test "$BOXMODEL" = "bre2ze4k")
AM_CONDITIONAL(BOXMODEL_VUSOLO4K, test "$BOXMODEL" = "vusolo4k")
AM_CONDITIONAL(BOXMODEL_VUDUO4K, test "$BOXMODEL" = "vuduo4k")
AM_CONDITIONAL(BOXMODEL_VUDUO4KSE, test "$BOXMODEL" = "vuduo4kse")
AM_CONDITIONAL(BOXMODEL_VUULTIMO4K, test "$BOXMODEL" = "vuultimo4k")
AM_CONDITIONAL(BOXMODEL_VUZERO4K, test "$BOXMODEL" = "vuzero4k")
AM_CONDITIONAL(BOXMODEL_VUUNO4KSE, test "$BOXMODEL" = "vuuno4kse")
AM_CONDITIONAL(BOXMODEL_VUUNO4K, test "$BOXMODEL" = "vuuno4k")
AM_CONDITIONAL(BOXMODEL_VUDUO, test "$BOXMODEL" = "vuduo")
AM_CONDITIONAL(BOXMODEL_VUDUO2, test "$BOXMODEL" = "vuduo2")
AM_CONDITIONAL(BOXMODEL_DM8000, test "$BOXMODEL" = "dm8000")
AM_CONDITIONAL(BOXMODEL_GB800SE, test "$BOXMODEL" = "gb800se")
AM_CONDITIONAL(BOXMODEL_OSNINO, test "$BOXMODEL" = "osnino")
AM_CONDITIONAL(BOXMODEL_OSNINOPLUS, test "$BOXMODEL" = "osninoplus")
AM_CONDITIONAL(BOXMODEL_OSNINOPRO, test "$BOXMODEL" = "osninopro")
AM_CONDITIONAL(BOXMODEL_H7, test "$BOXMODEL" = "h7")
AM_CONDITIONAL(BOXMODEL_OSMINI4K, test "$BOXMODEL" = "osmini4k")
AM_CONDITIONAL(BOXMODEL_OSMIO4K, test "$BOXMODEL" = "osmio4k")
AM_CONDITIONAL(BOXMODEL_OSMIO4KPLUS, test "$BOXMODEL" = "osmio4kplus")

AM_CONDITIONAL(BOXMODEL_RASPI, test "$BOXMODEL" = "raspi")

AM_CONDITIONAL(BOXMODEL_VUPLUS_ALL, test "$BOXMODEL" = "vusolo4k" -o "$BOXMODEL" = "vuduo4k" -o "$BOXMODEL" = "vuduo4kse" -o "$BOXMODEL" = "vuultimo4k" -o "$BOXMODEL" = "vuzero4k" -o "$BOXMODEL" = "vuuno4kse" -o "$BOXMODEL" = "vuuno4k" -o "$BOXMODEL" = "vuduo")
AM_CONDITIONAL(BOXMODEL_VUPLUS_ARM, test "$BOXMODEL" = "vusolo4k" -o "$BOXMODEL" = "vuduo4k" -o "$BOXMODEL" = "vuduo4kse" -o "$BOXMODEL" = "vuultimo4k" -o "$BOXMODEL" = "vuzero4k" -o "$BOXMODEL" = "vuuno4kse" -o "$BOXMODEL" = "vuuno4k")
AM_CONDITIONAL(BOXMODEL_VUPLUS_MIPS, test "$BOXMODEL" = "vuduo" -o "$BOXMODEL" = "vuduo2")

if test "$BOXTYPE" = "spark"; then
	AC_DEFINE(HAVE_SPARK_HARDWARE, 1, [building for a goldenmedia 990 or edision pingulux])
	AC_DEFINE(HAVE_SH4_HARDWARE, 1, [building for a sh4 box])
elif test "$BOXTYPE" = "duckbox"; then
	AC_DEFINE(HAVE_DUCKBOX_HARDWARE, 1, [building for a duckbox])
	AC_DEFINE(HAVE_SH4_HARDWARE, 1, [building for a sh4 box])
elif test "$BOXTYPE" = "generic"; then
	AC_DEFINE(HAVE_GENERIC_HARDWARE, 1, [building for a generic device like a standard PC])
elif test "$BOXTYPE" = "armbox"; then
	AC_DEFINE(HAVE_ARM_HARDWARE, 1, [building for an armbox])
elif test "$BOXTYPE" = "mipsbox"; then
	AC_DEFINE(HAVE_MIPS_HARDWARE, 1, [building for an mipsbox])
fi

# TODO: do we need more defines?
if test "$BOXMODEL" = "ufs910"; then
	AC_DEFINE(BOXMODEL_UFS910, 1, [ufs910])
elif test "$BOXMODEL" = "ufs912"; then
	AC_DEFINE(BOXMODEL_UFS912, 1, [ufs912])
elif test "$BOXMODEL" = "ufs913"; then
	AC_DEFINE(BOXMODEL_UFS913, 1, [ufs913])
elif test "$BOXMODEL" = "ufs922"; then
	AC_DEFINE(BOXMODEL_UFS922, 1, [ufs922])
elif test "$BOXMODEL" = "ufc960"; then
	AC_DEFINE(BOXMODEL_UFC960, 1, [ufc960])
elif test "$BOXMODEL" = "spark"; then
	AC_DEFINE(BOXMODEL_SPARK, 1, [spark])
elif test "$BOXMODEL" = "spark7162"; then
	AC_DEFINE(BOXMODEL_SPARK7162, 1, [spark7162])
elif test "$BOXMODEL" = "hs8200"; then
	AC_DEFINE(BOXMODEL_HS8200, 1, [hs8200])
elif test "$BOXMODEL" = "fs9000"; then
	AC_DEFINE(BOXMODEL_FS9000, 1, [fs9000])
elif test "$BOXMODEL" = "hs9510"; then
	AC_DEFINE(BOXMODEL_HS9510, 1, [hs9510])
elif test "$BOXMODEL" = "hs7110"; then
	AC_DEFINE(BOXMODEL_HS7110, 1, [hs7110])
elif test "$BOXMODEL" = "hs7119"; then
	AC_DEFINE(BOXMODEL_HS7119, 1, [hs7119])
elif test "$BOXMODEL" = "hs7420"; then
	AC_DEFINE(BOXMODEL_HS7420, 1, [hs7420])
elif test "$BOXMODEL" = "hs7429"; then
	AC_DEFINE(BOXMODEL_HS7119, 1, [hs7429])
elif test "$BOXMODEL" = "hs7810a"; then
	AC_DEFINE(BOXMODEL_HS7810A, 1, [hs7810a])
elif test "$BOXMODEL" = "hs7119"; then
	AC_DEFINE(BOXMODEL_HS7819, 1, [hs7819])
elif test "$BOXMODEL" = "cuberevo"; then
	AC_DEFINE(BOXMODEL_CUBEREVO, 1, [cuberevo])
elif test "$BOXMODEL" = "cuberevo_mini"; then
	AC_DEFINE(BOXMODEL_CUBEREVO_MINI, 1, [cuberevo_mini])
elif test "$BOXMODEL" = "cuberevo_mini2"; then
	AC_DEFINE(BOXMODEL_CUBEREVO_MINI2, 1, [cuberevo_mini2])
elif test "$BOXMODEL" = "cuberevo_250hd"; then
	AC_DEFINE(BOXMODEL_CUBEREVO_250HD, 1, [cuberevo_250hd])
elif test "$BOXMODEL" = "cuberevo_2000hd"; then
	AC_DEFINE(BOXMODEL_CUBEREVO_2000HD, 1, [cuberevo_2000hd])
elif test "$BOXMODEL" = "cuberevo_3000hd"; then
	AC_DEFINE(BOXMODEL_CUBEREVO_3000HD, 1, [cuberevo_3000hd])
elif test "$BOXMODEL" = "ipbox9900"; then
	AC_DEFINE(BOXMODEL_IPBOX9900, 1, [ipbox9900])
elif test "$BOXMODEL" = "ipbox99"; then
	AC_DEFINE(BOXMODEL_IPBOX99, 1, [ipbox99])
elif test "$BOXMODEL" = "ipbox55"; then
	AC_DEFINE(BOXMODEL_IPBOX55, 1, [ipbox55])
elif test "$BOXMODEL" = "tf7700"; then
	AC_DEFINE(BOXMODEL_TF7700, 1, [tf7700])
elif test "$BOXMODEL" = "atemio520"; then
	AC_DEFINE(BOXMODEL_ATEMIO520, 1, [atemio520])
elif test "$BOXMODEL" = "hl101"; then
	AC_DEFINE(BOXMODEL_HL101, 1, [hl101])
elif test "$BOXMODEL" = "vip1_v1"; then
	AC_DEFINE(BOXMODEL_VIP1_V1, 1, [vip1_v1])
elif test "$BOXMODEL" = "vip1_v2"; then
	AC_DEFINE(BOXMODEL_VIP1_V2, 1, [vip1_v2])
elif test "$BOXMODEL" = "vip2_v1"; then
	AC_DEFINE(BOXMODEL_VIP2, 1, [vip2])
elif test "$BOXMODEL" = "adb_box"; then
	AC_DEFINE(BOXMODEL_ADB_BOX, 1, [adb_box])
elif test "$BOXMODEL" = "adb_2850"; then
	AC_DEFINE(BOXMODEL_ADB_2850, 1, [adb_2850])
elif test "$BOXMODEL" = "vitamin_hd5000"; then
	AC_DEFINE(BOXMODEL_VITAMIN_HD5000, 1, [vitamin_hd5000])
elif test "$BOXMODEL" = "sagemcom88"; then
	AC_DEFINE(BOXMODEL_SAGEMCOM88, 1, [sagemcom88])
elif test "$BOXMODEL" = "arivalink200"; then
	AC_DEFINE(BOXMODEL_ARIVALINK200, 1, [arivalink200])
elif test "$BOXMODEL" = "pace7241"; then
	AC_DEFINE(BOXMODEL_PACE7241, 1, [pace7241])
elif test "$BOXMODEL" = "opt9600"; then
	AC_DEFINE(BOXMODEL_OPT9600, 1, [opt9600])
elif test "$BOXMODEL" = "opt9600mini"; then
	AC_DEFINE(BOXMODEL_OPT9600MINI, 1, [opt9600mini])
elif test "$BOXMODEL" = "opt9600prima"; then
	AC_DEFINE(BOXMODEL_OPT9600PRIMA, 1, [opt9600prima])
elif test "$BOXMODEL" = "bre2ze4k"; then
	AC_DEFINE(BOXMODEL_BRE2ZE4K, 1, [bre2ze4k])
elif test "$BOXMODEL" = "hd51"; then
	AC_DEFINE(BOXMODEL_HD51, 1, [hd51])
elif test "$BOXMODEL" = "multibox"; then
	AC_DEFINE(BOXMODEL_MULTIBOX, 1, [multibox])
elif test "$BOXMODEL" = "multiboxse"; then
	AC_DEFINE(BOXMODEL_MULTIBOXSE, 1, [multiboxse])
elif test "$BOXMODEL" = "hd60"; then
	AC_DEFINE(BOXMODEL_HD60, 1, [hd60])
elif test "$BOXMODEL" = "hd61"; then
	AC_DEFINE(BOXMODEL_HD61, 1, [hd61])
elif test "$BOXMODEL" = "vusolo4k"; then
	AC_DEFINE(BOXMODEL_VUSOLO4K, 1, [vusolo4k])
elif test "$BOXMODEL" = "vuduo4k"; then
	AC_DEFINE(BOXMODEL_VUDUO4K, 1, [vuduo4k])
elif test "$BOXMODEL" = "vuduo4kse"; then
	AC_DEFINE(BOXMODEL_VUDUO4KSE, 1, [vuduo4kse])
elif test "$BOXMODEL" = "vuultimo4k"; then
	AC_DEFINE(BOXMODEL_VUULTIMO4K, 1, [vuultimo4k])
elif test "$BOXMODEL" = "vuzero4k"; then
	AC_DEFINE(BOXMODEL_VUZERO4K, 1, [vuzero4k])
elif test "$BOXMODEL" = "vuuno4kse"; then
	AC_DEFINE(BOXMODEL_VUUNO4KSE, 1, [vuuno4kse])
elif test "$BOXMODEL" = "vuuno4k"; then
	AC_DEFINE(BOXMODEL_VUUNO4K, 1, [vuuno4k])
elif test "$BOXMODEL" = "vuduo"; then
	AC_DEFINE(BOXMODEL_VUDUO, 1, [vuduo])
elif test "$BOXMODEL" = "dm8000"; then
	AC_DEFINE(BOXMODEL_DM8000, 1, [dm8000])
elif test "$BOXMODEL" = "vuduo2"; then
	AC_DEFINE(BOXMODEL_VUDUO2, 1, [vuduo2])
elif test "$BOXMODEL" = "h7"; then
	AC_DEFINE(BOXMODEL_H7, 1, [h7])
elif test "$BOXMODEL" = "osmini4k"; then
	AC_DEFINE(BOXMODEL_OSMINI4K, 1, [osmini4k])
elif test "$BOXMODEL" = "osmio4k"; then
	AC_DEFINE(BOXMODEL_OSMIO4K, 1, [osmio4k])
elif test "$BOXMODEL" = "osmio4kplus"; then
	AC_DEFINE(BOXMODEL_OSMIO4KPLUS, 1, [osmio4kplus])
elif test "$BOXMODEL" = "gb800se"; then
	AC_DEFINE(BOXMODEL_GB800SE, 1, [gb800se])
elif test "$BOXMODEL" = "osnino"; then
	AC_DEFINE(BOXMODEL_OSNINO, 1, [osnino])
elif test "$BOXMODEL" = "osninoplus"; then
	AC_DEFINE(BOXMODEL_OSNINOPLUS, 1, [osninoplus])
elif test "$BOXMODEL" = "osninopro"; then
	AC_DEFINE(BOXMODEL_OSNINOPRO, 1, [osninopro])
elif test "$BOXMODEL" = "raspi"; then
	AC_DEFINE(BOXMODEL_RASPI, 1, [raspberry pi])
fi

# all vuplus BOXMODELs
case "$BOXMODEL" in
	vusolo4k|vuduo4k|vuduo4kse|vuultimo4k|vuuno4k|vuuno4kse|vuzero4k|vuduo|vuduo2)
		AC_DEFINE(BOXMODEL_VUPLUS_ALL, 1, [vuplus_all])
	;;
esac

# all vuplus arm BOXMODELs
case "$BOXMODEL" in
	vusolo4k|vuduo4k|vuduo4kse|vuultimo4k|vuuno4k|vuuno4kse|vuzero4k)
		AC_DEFINE(BOXMODEL_VUPLUS_ARM, 1, [vuplus_arm])
	;;
esac

# all vuplus mips BOXMODELs
case "$BOXMODEL" in
	vuduo|vuduo2)
		AC_DEFINE(BOXMODEL_VUPLUS_MIPS, 1, [vuplus_mips])
	;;
esac
])

dnl backward compatiblity
AC_DEFUN([AC_GNU_SOURCE], [
AH_VERBATIM([_GNU_SOURCE], [
/* Enable GNU extensions on systems that have them. */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
])dnl
AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_DEFINE([_GNU_SOURCE])
])

AC_DEFUN([AC_PROG_EGREP], [
AC_CACHE_CHECK([for egrep], [ac_cv_prog_egrep], [
if echo a | (grep -E '(a|b)') >/dev/null 2>&1; then
	ac_cv_prog_egrep='grep -E'
else
	ac_cv_prog_egrep='egrep'
fi
])
EGREP=$ac_cv_prog_egrep
AC_SUBST([EGREP])
])
