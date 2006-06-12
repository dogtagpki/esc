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

include manifest.mn

include $(CORE_DEPTH)/coreconf/config.mk

all:: importnss

include $(CORE_DEPTH)/coreconf/rules.mk

# only build the card applet on Windows, and if the tools are setup

ifeq ($(findstring WIN,$(OS_TARGET)),WIN)
ifneq (,$(JAVACARD_KIT_DIR))
ifneq (,$(JAVA_HOME))
ifneq (,$(SLB_DIR))
DIRS += applet
endif
endif
endif
endif

# import xulrunner to get the gdk


IMPORTS += src/xulrunner/v1.8.0.1/xulrunner-1.8.0.1-source.tar.gz
importnss:
ifeq ($(IMPORT_NSS),1)
ifneq ($(wildcard $(NSSDIR)/$(OBJDIR)/include/prtypes.h),$(NSSDIR)/$(OBJDIR)/include/prtypes.h)
	$(MAKE) import
endif
endif
