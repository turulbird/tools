AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	aio-grab \
	devinit \
	evremote2 \
	fp_control \
	fbshot \
	gitVCInfo \
	hd-idle \
	hotplug \
	libmme_host \
	libmme_image \
	showiframe \
	streamproxy \
	stfbcontrol \
	ustslave \
	vfdctl \
	wait4button

if BOXTYPE_TF7700
SUBDIRS += \
	tffpctl \
	tfd2mtd
endif

if BOXTYPE_CUBEREVO
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_MINI
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_MINI2
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_MINI_FTA
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_250HD
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_2000HD
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_3000HD
SUBDIRS += ipbox_eeprom
endif

if BOXTYPE_CUBEREVO_9500HD
SUBDIRS += ipbox_eeprom
endif

if ENABLE_EPLAYER3
if ENABLE_GSTREAMER
SUBDIRS += \
	libeplayer3
else
SUBDIRS += \
	eplayer3
endif
endif

if ENABLE_SPF
SUBDIRS += \
	spf_tool
endif
