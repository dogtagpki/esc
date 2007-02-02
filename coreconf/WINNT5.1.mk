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

#
# Config stuff for WINNT 5.1 (Windows XP)
#
# This makefile defines the following variables:
# OS_CFLAGS and OS_DLLFLAGS.

include $(CORE_DEPTH)/coreconf/WIN32.mk

ifeq ($(CPU_ARCH), x386)
	OS_CFLAGS += -W3 -nologo
	DEFINES += -D_X86_
else 
	ifeq ($(CPU_ARCH), MIPS)
		#OS_CFLAGS += -W3 -nologo
		#DEFINES += -D_MIPS_
		OS_CFLAGS += -W3 -nologo
	else 
		ifeq ($(CPU_ARCH), ALPHA)
			OS_CFLAGS += -W3 -nologo
			DEFINES += -D_ALPHA_=1
		endif
	endif
endif

OS_DLLFLAGS += -nologo -DLL -SUBSYSTEM:WINDOWS -PDB:NONE
#
# Win NT needs -GT so that fibers can work
#
OS_CFLAGS += -GT
DEFINES += -DWINNT

NSPR31_LIB_PREFIX = lib
