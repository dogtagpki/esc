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
# Master "Core Components" suffixes                                   #
#######################################################################

#
# Object suffixes   (OS2 and WIN% override this)
#
ifndef OBJ_SUFFIX
    OBJ_SUFFIX = .o
endif

#
# Assembler source suffixes (OS2 and WIN% override this)
#
ifndef ASM_SUFFIX
    ASM_SUFFIX = .s
endif

#
# Library suffixes
#
STATIC_LIB_EXTENSION =

ifndef DYNAMIC_LIB_EXTENSION
    DYNAMIC_LIB_EXTENSION =
endif


ifndef STATIC_LIB_SUFFIX
    STATIC_LIB_SUFFIX = .$(LIB_SUFFIX)
endif


ifndef DYNAMIC_LIB_SUFFIX
    DYNAMIC_LIB_SUFFIX = .$(DLL_SUFFIX)
endif

# WIN% overridese this
ifndef IMPORT_LIB_SUFFIX
    IMPORT_LIB_SUFFIX = 
endif


ifndef STATIC_LIB_SUFFIX_FOR_LINKING
    STATIC_LIB_SUFFIX_FOR_LINKING = $(STATIC_LIB_SUFFIX)
endif


# WIN% overridese this
ifndef DYNAMIC_LIB_SUFFIX_FOR_LINKING
    DYNAMIC_LIB_SUFFIX_FOR_LINKING = $(DYNAMIC_LIB_SUFFIX)
endif

#
# Program suffixes (OS2 and WIN% override this)
#

ifndef PROG_SUFFIX
    PROG_SUFFIX =
endif

MK_SUFFIX = included
