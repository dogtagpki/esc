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
# Build master "Core Components" release version directory name       #
#######################################################################

#
# Always set CURRENT_VERSION_SYMLINK to the <current> symbolic link.
#

CURRENT_VERSION_SYMLINK = current


#
#  For the sake of backwards compatibility (*sigh*) ...
#

ifndef VERSION
    ifdef BUILD_NUM
	VERSION = $(BUILD_NUM)
    endif
endif

ifndef RELEASE_VERSION
    ifdef BUILD_NUM
	RELEASE_VERSION = $(BUILD_NUM)
    endif
endif

#
# If VERSION has still NOT been set on the command line,
# as an environment variable, by the individual Makefile, or
# by the <component>-specific "version.mk" file, set VERSION equal
# to $(CURRENT_VERSION_SYMLINK).

ifndef VERSION
    VERSION = $(CURRENT_VERSION_SYMLINK)
endif

# If RELEASE_VERSION has still NOT been set on the command line,
# as an environment variable, by the individual Makefile, or
# by the <component>-specific "version.mk" file, automatically
# generate the next available version number via a perl script.
# 

ifndef RELEASE_VERSION
    RELEASE_VERSION = 
endif

#
# Set <component>-specific versions for compiliation and linkage.
#

ifndef JAVA_VERSION
    JAVA_VERSION = $(CURRENT_VERSION_SYMLINK)
endif

ifndef NETLIB_VERSION
    NETLIB_VERSION = $(CURRENT_VERSION_SYMLINK)
endif

ifndef NSPR_VERSION
    NSPR_VERSION = $(CURRENT_VERSION_SYMLINK)
endif

ifndef SECTOOLS_VERSION
    SECTOOLS_VERSION = $(CURRENT_VERSION_SYMLINK)
endif

ifndef SECURITY_VERSION
    SECURITY_VERSION = $(CURRENT_VERSION_SYMLINK)
endif

MK_VERSION = included
