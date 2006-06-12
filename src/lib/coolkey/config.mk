# ***** BEGIN COPYRIGHT BLOCK *****
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
# ***** END COPYRIGHT BLOCK *****

ifdef NISCC_TEST
DEFINES += -DNISCC_TEST
endif

ifeq (,$(filter-out WIN%,$(OS_TARGET)))

TARGETS = $(LIBRARY)
# don't want the 32 in the shared library name
SHARED_LIBRARY =
IMPORT_LIBRARY =

#RES = $(OBJDIR)/$(LIBRARY_NAME).res
#RESNAME = $(LIBRARY_NAME).rc

ifdef NS_USE_GCC
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib \
	-lhttpchunked \
	-lckyapplet \
	-lnss3 \
	-lplc4 \
	-lplds4 \
	-lnspr4 \
	$(NULL)
else # ! NS_USE_GCC
EXTRA_SHARED_LIBS += \
	$(DIST)/lib/httpchunked.lib \
	$(DIST)/lib/ckyapplet.lib \
	$(DIST)/lib/nss3.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plc4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plds4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)nspr4.lib \
	$(NULL)
endif # NS_USE_GCC

else


# $(PROGRAM) has NO explicit dependencies on $(EXTRA_SHARED_LIBS)
# $(EXTRA_SHARED_LIBS) come before $(OS_LIBS), except on AIX.
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib/ \
	-lhttpchunked \
	-lckyapplet \
	-lnss3 \
	-lplc4 \
	-lplds4 \
	-lnspr4 \
	-lssl3 \
	$(NULL)

ifeq ($(OS_ARCH), BeOS)
EXTRA_SHARED_LIBS += -lbe
endif

ifeq ($(OS_ARCH), Darwin)
EXTRA_SHARED_LIBS += -dylib_file @executable_path/libsoftokn3.dylib:$(DIST)/lib/libsoftokn3.dylib
endif

ifeq ($(OS_TARGET),SunOS)
# The -R '$ORIGIN' linker option instructs this library to search for its
# dependencies in the same directory where it resides.
MKSHLIB += -R '$$ORIGIN'
endif

endif
