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
# Master "Core Components" for computing program prefixes             #
#######################################################################

#
# Object prefixes
#

ifndef OBJ_PREFIX
    OBJ_PREFIX = 
endif

#
# Library suffixes
#

ifndef LIB_PREFIX
    LIB_PREFIX = lib
endif


ifndef DLL_PREFIX
    DLL_PREFIX = lib
endif


ifndef IMPORT_LIB_PREFIX
    IMPORT_LIB_PREFIX = 
endif

#
# Program prefixes
#

ifndef PROG_PREFIX
    PROG_PREFIX = 
endif

MK_PREFIX = included
