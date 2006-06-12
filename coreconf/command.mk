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
# Master "Core Components" default command macros;                    #
# can be overridden in <arch>.mk                                      #
#######################################################################

AS            = $(CC)
ASFLAGS      += $(CFLAGS)
CCF           = $(CC) $(CFLAGS)
LINK_DLL      = $(LINK) $(OS_DLLFLAGS) $(DLLFLAGS)
LINK_EXE      = $(LINK) $(OS_LFLAGS) $(LFLAGS)
NFSPWD        = $(NSINSTALL_DIR)/nfspwd
CFLAGS        = $(OPTIMIZER) $(OS_CFLAGS) $(XP_DEFINE) $(DEFINES) $(INCLUDES) \
		$(XCFLAGS)
RANLIB        = echo
TAR           = /bin/tar
#
# For purify
#
NOMD_CFLAGS  += $(OPTIMIZER) $(NOMD_OS_CFLAGS) $(XP_DEFINE) $(DEFINES) \
		$(INCLUDES) $(XCFLAGS)

MK_COMMAND = included
