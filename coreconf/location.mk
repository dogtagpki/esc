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
# Master "Core Components" macros to figure out binary code location  #
#######################################################################

#
# Figure out where the binary code lives.
#

ifdef BUILD_TREE
ifdef LIBRARY_NAME
BUILD         = $(BUILD_TREE)/netkey/$(LIBRARY_NAME)
OBJDIR        = $(BUILD_TREE)/netkey/$(LIBRARY_NAME)
DEPENDENCIES  = $(BUILD_TREE)/netkey/$(LIBRARY_NAME)/.md
else
BUILD         = $(BUILD_TREE)/netkey
OBJDIR        = $(BUILD_TREE)/netkey
DEPENDENCIES  = $(BUILD_TREE)/netkey/.md
endif
else
BUILD         = $(PLATFORM)
OBJDIR        = $(PLATFORM)
DEPENDENCIES  = $(PLATFORM)/.md
endif

DIST          = $(SOURCE_PREFIX)/$(PLATFORM)

ifdef BUILD_DEBUG_GC
    DEFINES += -DDEBUG_GC
endif

GARBAGE += $(DEPENDENCIES) core $(wildcard core.[0-9]*)

MK_LOCATION = included
