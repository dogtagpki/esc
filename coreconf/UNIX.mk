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

XP_DEFINE  += -DXP_UNIX
LIB_SUFFIX  = a
DLL_SUFFIX  = so
AR          = ar
AR         += cr $@
LDOPTS     += -L$(SOURCE_LIB_DIR)

ifdef BUILD_OPT
	OPTIMIZER  += -O
	DEFINES    += -UDEBUG -DNDEBUG
else
	OPTIMIZER  += -g
	DEFINES    += -DDEBUG -UNDEBUG -DDEBUG_$(shell whoami)
endif

ifdef BUILD_TREE
NSINSTALL_DIR  = $(BUILD_TREE)/netkey
NSINSTALL      = $(BUILD_TREE)/netkey/nsinstall
else
NSINSTALL_DIR  = $(CORE_DEPTH)/coreconf/nsinstall
NSINSTALL      = $(NSINSTALL_DIR)/$(OBJDIR_NAME)/nsinstall
endif

MKDEPEND_DIR    = $(CORE_DEPTH)/coreconf/mkdepend
MKDEPEND        = $(MKDEPEND_DIR)/$(OBJDIR_NAME)/mkdepend
MKDEPENDENCIES  = $(OBJDIR_NAME)/depend.mk

####################################################################
#
# One can define the makefile variable NSDISTMODE to control
# how files are published to the 'dist' directory.  If not
# defined, the default is "install using relative symbolic
# links".  The two possible values are "copy", which copies files
# but preserves source mtime, and "absolute_symlink", which
# installs using absolute symbolic links.  The "absolute_symlink"
# option requires NFSPWD.
#   - THIS IS NOT PART OF THE NEW BINARY RELEASE PLAN for 9/30/97
#   - WE'RE KEEPING IT ONLY FOR BACKWARDS COMPATIBILITY
####################################################################

ifeq ($(NSDISTMODE),copy)
	# copy files, but preserve source mtime
	INSTALL  = $(NSINSTALL)
	INSTALL += -t
else
	ifeq ($(NSDISTMODE),absolute_symlink)
		# install using absolute symbolic links
		INSTALL  = $(NSINSTALL)
		INSTALL += -L `$(NFSPWD)`
	else
		# install using relative symbolic links
		INSTALL  = $(NSINSTALL)
		INSTALL += -R
	endif
endif

define MAKE_OBJDIR
if test ! -d $(@D); then rm -rf $(@D); $(NSINSTALL) -D $(@D); fi
endef
