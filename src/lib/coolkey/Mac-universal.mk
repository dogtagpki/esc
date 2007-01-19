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

DARWIN_LIB_NAME=libckymanager.a

CORE_DEPTH=../../..
include $(CORE_DEPTH)/coreconf/config.mk

all libs:
 
	mkdir -p $(UNIVERSAL_OFFSET_PPC)
	mkdir -p $(UNIVERSAL_OFFSET_386)
	sudo gcc_select 4.0
	echo "Build i386."
	make -f common.mk OSX_ARCH_FLAGS="-arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"


	cp $(OBJDIR)/$(DARWIN_LIB_NAME) $(UNIVERSAL_OFFSET_386)
	make -f common.mk clean
	sudo gcc_select 3.3
	echo "Build ppc."
	make -f common.mk OSX_ARCH_FLAGS="-arch ppc" 
	cp $(OBJDIR)/$(DARWIN_LIB_NAME) $(UNIVERSAL_OFFSET_PPC)

	lipo -create $(UNIVERSAL_OFFSET_PPC)/$(DARWIN_LIB_NAME) $(UNIVERSAL_OFFSET_386)/$(DARWIN_LIB_NAME) -output $(OBJDIR)/$(DARWIN_LIB_NAME)

	ranlib $(OBJDIR)/$(DARWIN_LIB_NAME)
	make -f common.mk install

	sudo gcc_select 4.0

clean:
	"Darwing clean."
	make -f common.mk  clean
