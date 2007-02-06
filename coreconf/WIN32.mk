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
# Configuration common to all versions of Windows NT
# and Windows 95
#

DEFAULT_COMPILER = cl

ifdef NS_USE_GCC
	CC           = gcc
	CCC          = g++
	LINK         = ld
	AR           = ar
	AR          += cr $@
	RANLIB       = ranlib
	BSDECHO      = echo
	RC           = windres.exe -O coff --use-temp-file
	LINK_DLL      = $(CC) $(OS_DLLFLAGS) $(DLLFLAGS)
else
	CC           = cl
	CCC          = cl
	LINK         = link
	AR           = lib
	AR          += -NOLOGO -OUT:"$@"
	RANLIB       = echo
	BSDECHO      = echo
	RC           = rc.exe
endif

ifdef BUILD_TREE
NSINSTALL_DIR  = $(BUILD_TREE)/netkey
else
NSINSTALL_DIR  = $(CORE_DEPTH)/coreconf/nsinstall
endif
NSINSTALL      = nsinstall

MKDEPEND_DIR    = $(CORE_DEPTH)/coreconf/mkdepend
MKDEPEND        = $(MKDEPEND_DIR)/$(OBJDIR_NAME)/mkdepend.exe
# Note: MKDEPENDENCIES __MUST__ be a relative pathname, not absolute.
# If it is absolute, gmake will crash unless the named file exists.
MKDEPENDENCIES  = $(OBJDIR_NAME)/depend.mk

INSTALL      = $(NSINSTALL)
MAKE_OBJDIR  = mkdir
MAKE_OBJDIR += $(OBJDIR)
GARBAGE     += $(OBJDIR)/vc20.pdb $(OBJDIR)/vc40.pdb
XP_DEFINE   += -DXP_PC
ifdef NS_USE_GCC
LIB_SUFFIX   = a
else
LIB_SUFFIX   = lib
endif
DLL_SUFFIX   = dll

ifdef NS_USE_GCC
    OS_CFLAGS += -mno-cygwin -mms-bitfields
    _GEN_IMPORT_LIB=-Wl,--out-implib,$(IMPORT_LIBRARY)
    DLLFLAGS  += -mno-cygwin -o $@ -shared -Wl,--export-all-symbols $(if $(IMPORT_LIBRARY),$(_GEN_IMPORT_LIB))
    ifdef BUILD_OPT
	OPTIMIZER  += -O2
	DEFINES    += -UDEBUG -U_DEBUG -DNDEBUG
	#
	# Add symbolic information for a profiler
	#
	ifdef MOZ_PROFILE
		OPTIMIZER += -g
	endif
    else
	OPTIMIZER  += -g
	NULLSTRING :=
	SPACE      := $(NULLSTRING) # end of the line
	USERNAME   := $(subst $(SPACE),_,$(USERNAME))
	USERNAME   := $(subst -,_,$(USERNAME))
	DEFINES    += -DDEBUG -D_DEBUG -UNDEBUG -DDEBUG_$(USERNAME)
    endif
else # !NS_USE_GCC
	OS_CFLAGS  += -GX
    ifdef BUILD_OPT
	ifdef USE_STATIC_RTL
	OS_CFLAGS  += -MT
	else
	OS_CFLAGS  += -MD
	endif
	OPTIMIZER  += -O2
	DEFINES    += -UDEBUG -U_DEBUG -DNDEBUG
	DLLFLAGS   += -OUT:"$@"
	#
	# Add symbolic information for a profiler
	#
	ifdef MOZ_PROFILE
		OPTIMIZER += -Z7
		DLLFLAGS += -DEBUG -DEBUGTYPE:CV
	endif
    else
	#
	# Define USE_DEBUG_RTL if you want to use the debug runtime library
	# (RTL) in the debug build
	#
	ifdef USE_STATIC_RTL
	ifdef USE_DEBUG_RTL
		OS_CFLAGS += -MTd
	else
		OS_CFLAGS += -MT
	endif
	else
	ifdef USE_DEBUG_RTL
		OS_CFLAGS += -MDd
	else
		OS_CFLAGS += -MD
	endif
	endif
	OPTIMIZER  += -Od -Z7
	#OPTIMIZER += -Zi -Fd$(OBJDIR)/ -Od
	NULLSTRING :=
	SPACE      := $(NULLSTRING) # end of the line
	USERNAME   := $(subst $(SPACE),_,$(USERNAME))
	USERNAME   := $(subst -,_,$(USERNAME))
	DEFINES    += -DDEBUG -D_DEBUG -UNDEBUG -DDEBUG_$(USERNAME)
	DLLFLAGS   += -DEBUG -DEBUGTYPE:CV -OUT:"$@"
	LDFLAGS    += -DEBUG -DEBUGTYPE:CV -PDB:NONE
    endif
endif # NS_USE_GCC

DEFINES += -DWIN32
ifdef MAPFILE
ifndef NS_USE_GCC
DLLFLAGS += -DEF:$(MAPFILE)
endif
endif
# Change PROCESS to put the mapfile in the correct format for this platform
PROCESS_MAP_FILE = cp $(LIBRARY_NAME).def $@


#
#  The following is NOT needed for the NSPR 2.0 library.
#

DEFINES += -D_WINDOWS

# override default, which is ASFLAGS = CFLAGS
ifdef NS_USE_GCC
	AS	= $(CC)
	ASFLAGS = $(INCLUDES)
else
	AS	= ml.exe
	ASFLAGS = -Cp -Sn -Zi -coff $(INCLUDES)
endif

#
# override the definitions of RELEASE_TREE found in tree.mk
#
ifndef RELEASE_TREE
    ifdef BUILD_SHIP
	ifdef USE_SHIPS
	    RELEASE_TREE = $(NTBUILD_SHIP)
	else
	    RELEASE_TREE = 
	endif
    else
	RELEASE_TREE =
    endif
endif

#
# override the definitions of IMPORT_LIB_PREFIX, LIB_PREFIX, and
# DLL_PREFIX in prefix.mk
#

ifndef IMPORT_LIB_PREFIX
    ifdef NS_USE_GCC
	IMPORT_LIB_PREFIX = lib
    else
	IMPORT_LIB_PREFIX = $(NULL)
    endif
endif

ifndef LIB_PREFIX
    ifdef NS_USE_GCC
	LIB_PREFIX = lib
    else
	LIB_PREFIX = $(NULL)
    endif
endif

ifndef DLL_PREFIX
    DLL_PREFIX =  $(NULL)
endif

#
# override the definitions of various _SUFFIX symbols in suffix.mk
#

#
# Object suffixes
#
ifndef OBJ_SUFFIX
    ifdef NS_USE_GCC
	OBJ_SUFFIX = .o
    else
	OBJ_SUFFIX = .obj
    endif
endif

#
# Assembler source suffixes
#
ifndef ASM_SUFFIX
    ifdef NS_USE_GCC
	ASM_SUFFIX = .s
    else
	ASM_SUFFIX = .asm
    endif
endif

#
# Library suffixes
#

ifndef IMPORT_LIB_SUFFIX
    IMPORT_LIB_SUFFIX = .$(LIB_SUFFIX)
endif

ifndef DYNAMIC_LIB_SUFFIX_FOR_LINKING
    DYNAMIC_LIB_SUFFIX_FOR_LINKING = $(IMPORT_LIB_SUFFIX)
endif

#
# Program suffixes
#
ifndef PROG_SUFFIX
    PROG_SUFFIX = .exe
endif

#
# When the processor is NOT 386-based on Windows NT, override the
# value of $(CPU_TAG).  For WinNT, 95, 16, not CE.
#
ifneq ($(CPU_ARCH),x386)
    CPU_TAG = _$(CPU_ARCH)
endif

#
# override ruleset.mk, removing the "lib" prefix for library names, and
# adding the "32" after the LIBRARY_VERSION.
#
ifdef LIBRARY_NAME
    SHARED_LIBRARY = $(OBJDIR)/$(LIBRARY_NAME)$(LIBRARY_VERSION)32$(JDK_DEBUG_SUFFIX).dll
    IMPORT_LIBRARY = $(OBJDIR)/$(LIBRARY_NAME)$(LIBRARY_VERSION)32$(JDK_DEBUG_SUFFIX).lib
endif

#
# override the TARGETS defined in ruleset.mk, adding IMPORT_LIBRARY
#
ifndef TARGETS
    TARGETS = $(LIBRARY) $(SHARED_LIBRARY) $(IMPORT_LIBRARY) $(PROGRAM) $(SCRIPTS)
endif

