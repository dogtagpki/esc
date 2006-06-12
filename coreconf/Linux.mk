# BEGIN COPYRIGHT BLOCK
# This Program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 of the License.
#
# This Program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA.
#
# Copyright (C) 2005 Red Hat, Inc.
# All rights reserved.
# END COPYRIGHT BLOCK

#
# Config stuff for Linux
#

include $(CORE_DEPTH)/coreconf/UNIX.mk

#
# The default implementation strategy for Linux is now pthreads
#
USE_PTHREADS = 1

ifeq ($(USE_PTHREADS),1)
	IMPL_STRATEGY = _PTH
endif

CC			= gcc
CCC			= g++
RANLIB			= ranlib

DEFAULT_COMPILER = gcc

ifeq ($(OS_TEST),m68k)
	OS_REL_CFLAGS	= -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH	= m68k
else		
ifeq ($(OS_TEST),ppc)
	OS_REL_CFLAGS	= -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH	= ppc
else
ifeq ($(OS_TEST),alpha)
        OS_REL_CFLAGS   = -D_ALPHA_ -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH	= alpha
else
ifeq ($(OS_TEST),ia64)
	OS_REL_CFLAGS	= -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH	= ia64
else
ifeq ($(OS_TEST),sparc)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = sparc
else
ifeq ($(OS_TEST),sparc64)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = sparc
else
ifeq (,$(filter-out arm% sa110,$(OS_TEST)))
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = arm
else
ifeq ($(OS_TEST),parisc64)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = hppa
else
ifeq ($(OS_TEST),s390)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = s390
else
ifeq ($(OS_TEST),s390x)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = s390x
else
ifeq ($(OS_TEST),mips)
	OS_REL_CFLAGS   = -DLINUX1_2 -D_XOPEN_SOURCE
	CPU_ARCH        = mips
else
	OS_REL_CFLAGS	= -DLINUX1_2 -Di386 -D_XOPEN_SOURCE
	CPU_ARCH	= x86
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif


LIBC_TAG		= _glibc

ifeq ($(OS_RELEASE),2.0)
	OS_REL_CFLAGS	+= -DLINUX2_0
	MKSHLIB		= $(CC) -shared -Wl,-soname -Wl,$(@:$(OBJDIR)/%.so=%.so)
	ifdef BUILD_OPT
		OPTIMIZER	= -O2
	endif
	ifdef MAPFILE
		MKSHLIB += -Wl,--version-script,$(MAPFILE)
	endif
	PROCESS_MAP_FILE = grep -v ';-' $(LIBRARY_NAME).def | \
         sed -e 's,;+,,' -e 's; DATA ;;' -e 's,;;,,' -e 's,;.*,;,' > $@
endif

ifeq ($(USE_PTHREADS),1)
OS_PTHREAD = -lpthread 
endif

OS_CFLAGS		= $(DSO_CFLAGS) $(OS_REL_CFLAGS) -ansi -Wall -pipe -DLINUX -Dlinux -D_POSIX_SOURCE -D_BSD_SOURCE -DHAVE_STRERROR
OS_LIBS			= -L/lib $(OS_PTHREAD) -ldl -lc

ifdef USE_PTHREADS
	DEFINES		+= -D_REENTRANT
endif

ARCH			= linux

DSO_CFLAGS		= -fPIC
DSO_LDOPTS		= -shared
DSO_LDFLAGS		=

# INCLUDES += -I/usr/include -Y/usr/include/linux
G++INCLUDES		= -I/usr/include/g++

#
# Always set CPU_TAG on Linux, OpenVMS, WINCE.
#
CPU_TAG = _$(CPU_ARCH)
