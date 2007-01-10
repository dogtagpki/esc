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
# Config stuff for Darwin.
#

include $(CORE_DEPTH)/coreconf/UNIX.mk

AR          = libtool
AR         += -static -o $@

DEFINES += -DMAC

INCLUDES += -I/System/Library/Frameworks/PCSC.framework/Versions/Current/Headers

DEFAULT_COMPILER = cc

CC		= cc
CCC		= c++
RANLIB		= ranlib

ifeq (86,$(findstring 86,$(OS_TEST)))
OS_REL_CFLAGS	= -Di386
CPU_ARCH	= i386
else
OS_REL_CFLAGS	= -Dppc
CPU_ARCH	= ppc
endif

# "Commons" are tentative definitions in a global scope, like this:
#     int x;
# The meaning of a common is ambiguous.  It may be a true definition:
#     int x = 0;
# or it may be a declaration of a symbol defined in another file:
#     extern int x;
# Use the -fno-common option to force all commons to become true
# definitions so that the linker can catch multiply-defined symbols.
# Also, common symbols are not allowed with Darwin dynamic libraries.

OS_CFLAGS	= $(DSO_CFLAGS) $(OS_REL_CFLAGS) -Wmost -fpascal-strings -no-cpp-precomp -fno-common -pipe -DDARWIN -DHAVE_STRERROR -DHAVE_BSD_FLOCK

ifdef BUILD_OPT
OPTIMIZER	= -O2
endif

ARCH		= darwin

# May override this with -bundle to create a loadable module.
DSO_LDOPTS	= -dynamiclib -compatibility_version 1 -current_version 1 -install_name @executable_path/$(notdir $@)

MKSHLIB		= $(CC) -arch $(CPU_ARCH) $(DSO_LDOPTS)
DLL_SUFFIX	= dylib
PROCESS_MAP_FILE = grep -v ';+' $(LIBRARY_NAME).def | grep -v ';-' | \
                sed -e 's; DATA ;;' -e 's,;;,,' -e 's,;.*,,' -e 's,^,_,' > $@

G++INCLUDES	= -I/usr/include/g++
#OS_LIBS += /usr/lib/libstdc++.a
#EXTRA_LIBS += /usr/lib/libstdc++.a

