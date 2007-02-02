#! gmake
#
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


#######################################################################
# (1) Include initial platform-independent assignments (MANDATORY).   #
#######################################################################

include manifest.mn

include config.mk

#######################################################################
# (2) Include "global" configuration information. (OPTIONAL)          #
#######################################################################

include $(CORE_DEPTH)/coreconf/config.mk

#only want the library
SHARED_LIBRARY=
IMPORT_LIBRARY=

#######################################################################
# (3) Include "component" configuration information. (OPTIONAL)       #
#######################################################################

DEFINES += -DDLL_PREFIX=\"$(DLL_PREFIX)\"
DEFINES += -DDLL_SUFFIX=\"$(DLL_SUFFIX)\"

ifdef DARWIN_GCC_VERSION
        echo "blooie"
        sudo gcc_select $(GCC_VERSION)
endif

#######################################################################
# (4) Include "local" platform-dependent assignments (OPTIONAL).      #
#######################################################################

#include config.mk

#######################################################################
# (5) Execute "global" rules. (OPTIONAL)                              #
#######################################################################

include $(CORE_DEPTH)/coreconf/rules.mk

#######################################################################
# (6) Execute "component" rules. (OPTIONAL)                           #
#######################################################################



#######################################################################
# (7) Execute "local" rules. (OPTIONAL).                              #
#######################################################################

UNIVERSAL_OFFSET=
ifeq ($(OS_ARCH), Darwin)

UNIVERSAL_OFFSET=i386
XULRUNNER_BASE=$(CORE_DEPTH)/dist/$(OBJDIR)//xulrunner_build/$(UNIVERSAL_OFFSET)

DEFINES += -I$(XULRUNNER_BASE)/dist/public/nss -I$(XULRUNNER_BASE)/dist/include/nspr
CFLAGS +=  $(OSX_ARCH_FLAGS)  

endif
ifdef CKY_INCLUDE
CFLAGS +=  $(CKY_INCLUDE)
endif

