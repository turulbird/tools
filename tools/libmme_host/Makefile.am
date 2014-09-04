AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS = -Wall -D__LINUX__

if ENABLE_MULTICOM324
lib_LTLIBRARIES = libmme_host.la

libmme_host_la_SOURCES = ../../../driver/multicom/mme/mme_linux_user.c \
			 ../../../driver/multicom/mme/ExecutionLoop.c \
			 ../../../driver/multicom/mme/LocalTransformer.c \
			 ../../../driver/multicom/mme/LookupTable.c \
			 ../../../driver/multicom/mme/MMEMessageQueue.c \
			 ../../../driver/multicom/mme/mme_tune.c \
			 ../../../driver/multicom/embxshell/embx_linux.c
endif
