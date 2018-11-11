AUTOMAKE_OPTIONS = subdir-objects

noinst_LIBRARIES = libeplayer3.a

AM_CPPFLAGS = -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
AM_CPPFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
AM_CPPFLAGS += -I$(srcdir)/include -I$(top_srcdir)/include
AM_CXXFLAGS = -fno-rtti -fno-exceptions -fno-strict-aliasing

AM_LDFLAGS = -lpthread -lrt

libeplayer3_a_SOURCES = \
	input.cpp output.cpp manager.cpp player.cpp \
	writer/writer.cpp \
	writer/pes.cpp \
	writer/misc.cpp \
	thread/condition_abstraction.cpp \
	thread/reentrant_mutex.cpp \
	thread/scoped_lock.cpp \
	thread/thread_abstraction.cpp \
	thread/mutex_abstraction.cpp
# writer/wmv.cpp writer/ac3.cpp writer/divx.cpp
# writer/dts.cpp writer/mpeg2.cpp writer/mp3.cpp
# writer/h264.cpp
# writer/h263.cpp writer/vc1.cpp writer/pcm.cpp writer/aac.cpp

LIBEPLAYER3_LIBS = libeplayer3.a
#lib_LTLIBRARIES = libeplayer3.la
# -lpthread -lavformat -lavcodec -lavutil -lswresample -lm

