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
# Master "Core Components" file system "release" prefixes             #
#######################################################################

# Windows platforms override this.  See WIN32.mk or WIN16.mk.
ifndef RELEASE_TREE
    ifdef BUILD_SHIP
	ifdef USE_SHIPS 
	    RELEASE_TREE = $(BUILD_SHIP)
	else
	    RELEASE_TREE = /share/builds/components
	endif
    else
	RELEASE_TREE = /share/builds/components
    endif
endif

#
# NOTE:  export control policy enforced for XP and MD files
#        released to the binary release tree
#

ifeq ($(POLICY), domestic)
    RELEASE_XP_DIR = domestic
    RELEASE_MD_DIR = domestic/$(PLATFORM)
else
    ifeq ($(POLICY), export)
	RELEASE_XP_DIR = export
	RELEASE_MD_DIR = export/$(PLATFORM)
    else
	ifeq ($(POLICY), france)
	    RELEASE_XP_DIR = france
	    RELEASE_MD_DIR = france/$(PLATFORM)
	else
	    RELEASE_XP_DIR = 
	    RELEASE_MD_DIR = $(PLATFORM)
	endif
    endif
endif


REPORTER_TREE = $(subst \,\\,$(RELEASE_TREE))

IMPORT_XP_DIR = 
IMPORT_MD_DIR = $(PLATFORM)

MK_TREE = included
