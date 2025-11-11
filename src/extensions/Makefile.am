# SPDX-License-Identifier: GPL-2.0-or-later

# OpenOCD Extensions Library
# This library contains extended features that are not part of the core OpenOCD functionality

noinst_LTLIBRARIES += %D%/libextensions.la

%C%_libextensions_la_SOURCES = \
	%D%/vfs_memory.c \
	%D%/vfs_crypto.c \
	%D%/aes.c \
	%D%/jim_source_hook.c \
	%D%/jim_vfs_wrapper.c \
	%D%/vfs_memory.h \
	%D%/vfs_crypto.h \
	%D%/aes.h \
	%D%/jim_source_hook.h \
	%D%/jim_vfs_wrapper.h

# Make headers available to other modules
# Note: We use += to append to AM_CPPFLAGS which already includes jimtcl paths
%C%_libextensions_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src
