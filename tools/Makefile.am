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
	ipbox_eeprom \
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
