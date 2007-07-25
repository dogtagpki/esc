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

CORE_DEPTH = ../../..
include $(CORE_DEPTH)/coreconf/config.mk
DIST = $(CORE_DEPTH)/dist/$(OBJDIR_NAME)

ifeq ($(OS_ARCH), WINNT)
WINNT = 1
endif

ifeq ($(OS_ARCH), Darwin)
MACOS = 1
endif 

ifndef BUILD_OPT
ifdef WINNT
CFLAGS = -Z7
else
CFLAGS = -g
endif
CFLAGS += -DDEBUG
endif


ifdef WINNT
LIBS = \
	version.lib \
	Setupapi.lib \
	Advapi32.lib \
	$(NULL)

all: $(OBJDIR_NAME)/eginstall.exe


import:
	@echo "Nothing to Import"

clean:
	rm -f $(OBJDIR_NAME)/eginstall.exe $(OBJDIR_NAME)/eginstall.obj

libs: $(OBJDIR_NAME)/eginstall.exe


install:
	mkdir -p $(DIST)/coolkey_drivers/egate/
	cp $(OBJDIR_NAME)/eginstall.exe $(DIST)/coolkey_drivers/egate/


$(OBJDIR_NAME)/eginstall.exe: $(OBJDIR_NAME)/eginstall.obj
	mkdir -p $(OBJDIR_NAME)
	cl $(CFLAGS) -o $(OBJDIR_NAME)/eginstall.exe $(OBJDIR_NAME)/eginstall.obj $(LIBS)
# /link /subsystem:WINDOWS
	editbin /subsystem:WINDOWS $(OBJDIR_NAME)/eginstall.exe

$(OBJDIR_NAME)/eginstall.obj: eginstall.c
	mkdir -p $(OBJDIR_NAME)
	cl -c -Fo$@ -DWINDOWS $(CFLAGS) eginstall.c

else
# MAC & Linux PC/SC-lite installer

ifdef MACOS
LIBS +=  -framework  Security -framework Carbon
CFLAGS += -DMAC
RESTART = $(OBJDIR_NAME)/pcscd_restart
endif

COMMON_OBJS=$(OBJDIR_NAME)/instlib.o

all: $(OBJDIR_NAME)/eginstall $(RESTART)

import:
	@echo "Nothing to Import"


clean:
	rm -f $(OBJDIR_NAME)/eginstall $(COMMON_OBJS) $(OBJDIR_NAME)/eginstall.o $(OBJDIR_NAME)/pcscd_restart.sh $(OBJDIR_NAME)/pcscd_restart $(OBJDIR_NAME)/pcscd_restart.o

libs: $(OBJDIR_NAME)/eginstall $(RESTART)

install: libs
	mkdir -p $(DIST)/coolkey_drivers/
	cp $(OBJDIR_NAME)/eginstall $(DIST)/coolkey_drivers/
ifdef MACOS
	cp $(OBJDIR_NAME)/pcscd_restart $(DIST)/coolkey_drivers/
	rm -f $(OBJDIR_NAME)/"COOL Key Install"
	ln $(OBJDIR_NAME)/eginstall $(OBJDIR_NAME)/"COOL Key Install"
	rm -f $(DIST)/coolkey_drivers/"COOL Key Install"
	ln $(DIST)/coolkey_drivers/eginstall $(DIST)/coolkey_drivers/"COOL Key Install"
endif

$(OBJDIR_NAME)/%.o: %.c
	mkdir -p $(OBJDIR_NAME)
	cc -c -o $@ $(CFLAGS)  $<

$(OBJDIR_NAME)/eginstall: $(OBJDIR_NAME)/eginstall.o $(COMMON_OBJS)
	mkdir -p $(OBJDIR_NAME)
	cc -o $(OBJDIR_NAME)/eginstall $< $(COMMON_OBJS) $(LIBS)

$(OBJDIR_NAME)/pcscd_restart: $(OBJDIR_NAME)/pcscd_restart.o $(COMMON_OBJS)
	mkdir -p $(OBJDIR_NAME)
	cc -o $(OBJDIR_NAME)/pcscd_restart $< $(COMMON_OBJS) $(LIBS)


$(OBJDIR_NAME)/pcscd_restart.sh:	pcscd_restart.sh.$(OS_ARCH)
	cp $< $@

endif

export:
	@echo "Nothing to Export"
