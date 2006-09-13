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

CORE_DEPTH = ../../../..
include $(CORE_DEPTH)/coreconf/config.mk
include $(CORE_DEPTH)/coreconf/rules.mk

CORE_INC=$(SOURCE_XP_DIR)/public
MOZ_SRCDIR=$(SOURCE_XP_DIR)/src/mozilla
MOZ_OBJDIR=$(DIST)/xulrunner_build

ifeq ($(OS_ARCH), Darwin)
#MOZ_OBJDIR=$(MOZ_SRCDIR)
endif

ifeq ($(OS_ARCH),Linux)
endif

all export install clean deploy args::  
	echo "***** Dropping into the Mozilla build environment ***** $(MOZ_OBJDIR) $@"
	echo $(DIST)
	$(MAKE) -f Makefile.moz $@ \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJDIR=$(MOZ_OBJDIR) \

libs::

	$(MAKE) -f Makefile.moz libs \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJ_DIR=$(MOZ_OBJDIR) \

	$(MAKE) -f Makefile.moz deploy \
		OBJDIR=$(OBJDIR) \
		CORE_DIST=$(DIST) \
		CORE_INC=$(CORE_INC) \
		MOZ_SRCDIR=$(MOZ_SRCDIR) \
		MOZ_OBJ_DIR=$(MOZ_OBJDIR)


