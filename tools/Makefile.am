AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	aio-grab \
	devinit \
	evremote2 \
	fp_control \
	fbshot \
	showiframe \
	streamproxy \
	stfbcontrol \
	ustslave \
	gitVCInfo \
	hotplug \
	vfdctl \
	hd-idle \
	libmme_host \
	libmme_image \
	wait4button

if BOXTYPE_TF7700
SUBDIRS += \
	tffpctl \
	tfd2mtd
endif

if ENABLE_CUBEREVO
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_MINI
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_MINI2
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_MINI_FTA
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_250HD
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_2000HD
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_3000HD
SUBDIRS += ipbox_eeprom
endif

if ENABLE_CUBEREVO_9500HD
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
