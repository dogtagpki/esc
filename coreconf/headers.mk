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
# Master "Core Components" include switch for support header files    #
#######################################################################

#
#  Always append source-side machine-dependent (md) and cross-platform
#  (xp) include paths
#

INCLUDES  += -I$(SOURCE_MDHEADERS_DIR) 

ifneq ($(OS_TARGET),WIN16)
    INCLUDES  += -I$(SOURCE_XPHEADERS_DIR)
endif

#
#  Only append source-side private cross-platform include paths for
#  sectools
#

INCLUDES += -I$(SOURCE_XPPRIVATE_DIR)

ifdef MOZILLA_CLIENT
    INCLUDES += -I$(SOURCE_XP_DIR)/include $(MOZILLA_INCLUDES)
endif

MK_HEADERS = included
