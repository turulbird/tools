AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

subdirs = external/flv2mpeg4

AM_CFLAGS = -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
AM_CFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
AM_CFLAGS += -I$(srcdir)/include -I$(srcdir)/external/ffmpeg -I$(top_srcdir)/include
AM_CXXFLAGS = -fno-rtti -fno-exceptions -fno-strict-aliasing
AM_LDFLAGS = -lpthread -lrt -lavformat -lavcodec -lavutil -lswresample

lib_LTLIBRARIES = libeplayer3.la

libeplayer3_la_SOURCES = \
	container/container.c \
	container/container_ffmpeg.c \
	external/ffmpeg/src/bitstream.c \
	external/ffmpeg/src/latmenc.c \
	external/ffmpeg/src/mpeg4audio.c \
	manager/manager.c \
	manager/audio.c \
	manager/video.c \
	manager/subtitle.c \
	output/output_subtitle.c \
	output/output.c \
	output/writer/common/pes.c \
	output/writer/common/misc.c \
	output/writer/common/writer.c \
	output/linuxdvb_buffering.c \
	playback/playback.c \
	output/linuxdvb_sh4.c \
	output/writer/sh4/writer.c \
	output/writer/sh4/aac.c \
	output/writer/sh4/ac3.c \
	output/writer/sh4/divx2.c \
	output/writer/sh4/dts.c \
	output/writer/sh4/h263.c \
	output/writer/sh4/h264.c \
	output/writer/sh4/mp3.c \
	output/writer/sh4/mpeg2.c \
	output/writer/sh4/pcm.c \
	output/writer/sh4/vc1.c \
	output/writer/sh4/wma.c \
	output/writer/sh4/wmv.c

bin_PROGRAMS = exteplayer3
#bin_PROGRAMS = exteplayer3 flv2mpeg4

exteplayer3_SOURCES = main/exteplayer.c
#exteplayer3_LDADD = -leplayer3 -lpthread -lass -lm -lpng
exteplayer3_LDADD = -leplayer3 -lpthread
exteplayer3_DEPENDENCIES = libeplayer3.la

#flv2mpeg4_SOURCES = 
#	external/flv2mpeg4/src/dcprediction.c 
#	?/avformat_writer.c 
#	external/flv2mpeg4/src/flv2mpeg4.c 
#	?/fetch.c 
#	external/flv2mpeg4/src/flvdecoder.c 
#	external/flv2mpeg4/src/m4vencode.c 
#	?/mp3header.c
#flv2mpeg4_DEPENDENCIES = libeplayer3.la


