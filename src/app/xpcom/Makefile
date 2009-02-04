#! gmake
# 
# ***** BEGIN COPYRIGHT BLOCK *****
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
# ***** END COPYRIGHT BLOCK *****

CORE_DEPTH = ../../..

MAKEFILE=Makefile.moz


ifdef USE_XUL_SDK
MAKEFILE=Makefile.sdk
endif

include $(CORE_DEPTH)/coreconf/config.mk
include $(CORE_DEPTH)/coreconf/rules.mk

CORE_INC=$(SOURCE_XP_DIR)/public
MOZ_SRCDIR=/home/jmagne/xulrunner/mozilla
#MOZ_OBJDIR=$(DIST)/xulrunner_build
MOZ_OBJDIR=$(MOZ_SRC_DIR)

DARWIN_LIB_NAME=librhCoolKey.dylib

UNIVERSAL_OFFSET_PPC=ppc
UNIVERSAL_OFFSET_386=i386
MOZ_OBJDIR_PPC=
ifeq ($(OS_ARCH),Darwin)
MOZ_OBJDIR=$(DIST)/xulrunner_build/$(UNIVERSAL_OFFSET_386)
MOZ_OBJDIR_PPC=$(DIST)/xulrunner_build/$(UNIVERSAL_OFFSET_PPC)
endif



ifeq ($(OS_ARCH),Linux)
endif


all export install clean deploy args::  
	echo "***** Dropping into the Mozilla build environment ***** $(MOZ_OBJDIR)"
	$(MAKE) -f $(MAKEFILE)  $@ \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJDIR=$(MOZ_OBJDIR) \

ifeq ($(OS_ARCH),Darwin)
		echo "Darwin all"
		$(MAKE) -f $(MAKEFILE) $@ \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJDIR=$(MOZ_OBJDIR_PPC)
endif


libs::
	echo "***** Dropping into the Mozilla build environment ***** $(MOZ_OBJDIR)"
	$(MAKE) -f $(MAKEFILE) libs \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJDIR=$(MOZ_OBJDIR) \

ifeq ($(OS_ARCH),Darwin)
		echo "Darwin libs"
		mkdir -p $(UNIVERSAL_OFFSET_386)
		mkdir -p $(UNIVERSAL_OFFSET_PPC)
		mv -n $(DARWIN_LIB_NAME) i386
		rm Makefile.in
		rm *.o
		$(MAKE) -f $(MAKEFILE) libs \
			OBJDIR=$(OBJDIR) \
			CORE_DIST=$(DIST) \
			CORE_INC=$(CORE_INC) \
			MOZ_SRCDIR=$(MOZ_SRCDIR) \
			MOZ_OBJDIR=$(MOZ_OBJDIR_PPC) \
			PPC_BUILD=1
			mv $(DARWIN_LIB_NAME) $(UNIVERSAL_OFFSET_PPC)
endif


ifeq ($(OS_ARCH),Darwin)
	echo "Darwin Creating UNI binary."
	lipo -create $(UNIVERSAL_OFFSET_PPC)/$(DARWIN_LIB_NAME) $(UNIVERSAL_OFFSET_386)/$(DARWIN_LIB_NAME) -output $(DARWIN_LIB_NAME)
endif
	$(MAKE) -f $(MAKEFILE) deploy \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJDIR=$(MOZ_OBJDIR)
