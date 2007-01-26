#! gmake
#
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

UNIVERSAL_OFFSET_PPC=ppc
UNIVERSAL_OFFSET_386=i386

CORE_DEPTH=../../..
include $(CORE_DEPTH)/coreconf/config.mk

include $(CORE_DEPTH)/coreconf/rules.mk

libs::

ifeq ($(OS_ARCH),Darwin)

	echo "Build Darwin."
	make -f Mac-universal.mk
else
	echo "Build Linux or Windows."
	make -f common.mk

endif

ifeq ($(OS_ARCH),Darwin)
clean::
	echo "Cleaning."
	rm -rf $(UNIVERSAL_OFFSET_PPC)
	rm -rf $(UNIVERSAL_OFFSET_386)
	make -f common.mk clean
endif
