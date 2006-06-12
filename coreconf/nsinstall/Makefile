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

DEPTH		= ../..
CORE_DEPTH	= ../..

MODULE		= coreconf

CSRCS		= nsinstall.c pathsub.c

PLSRCS		= nfspwd.pl

PROGRAM		= nsinstall

# Indicate that this directory builds build tools.
INTERNAL_TOOLS  = 1


include $(DEPTH)/coreconf/config.mk

ifeq (,$(filter-out OS2 WIN%,$(OS_TARGET)))
PROGRAM		=
else
TARGETS		= $(PROGRAM) $(PLSRCS:.pl=)
endif

ifdef NATIVE_CC
CC=$(NATIVE_CC)
endif

ifdef NATIVE_FLAGS
OS_CFLAGS=$(NATIVE_FLAGS)
endif

include $(DEPTH)/coreconf/rules.mk

# Redefine MAKE_OBJDIR for just this directory
define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); mkdir $(@D); fi
endef

