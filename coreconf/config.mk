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
# Configuration information for building in the "Core Components" source module
#

#######################################################################
# [1.0] Master "Core Components" source and release <architecture>    #
#       tags                                                          #
#######################################################################
ifndef MK_ARCH
include $(CORE_DEPTH)/coreconf/arch.mk
endif

#######################################################################
# [2.0] Master "Core Components" default command macros               #
#       (NOTE: may be overridden in $(OS_TARGET)$(OS_RELEASE).mk)     #
#######################################################################
ifndef MK_COMMAND
include $(CORE_DEPTH)/coreconf/command.mk
endif

#######################################################################
# [3.0] Master "Core Components" <architecture>-specific macros       #
#       (dependent upon <architecture> tags)                          #
#                                                                     #
#       We are moving towards just having a $(OS_TARGET).mk file      #
#       as opposed to multiple $(OS_TARGET)$(OS_RELEASE).mk files,    #
#       one for each OS release.                                      #
#######################################################################

TARGET_OSES = FreeBSD BSD_OS NetBSD OpenUNIX OS2 QNX Darwin BeOS OpenBSD \
              OpenVMS

ifeq (,$(filter-out $(TARGET_OSES),$(OS_TARGET)))
include $(CORE_DEPTH)/coreconf/$(OS_TARGET).mk
else
include $(CORE_DEPTH)/coreconf/$(OS_TARGET)$(OS_RELEASE).mk
endif

#######################################################################
# [4.0] Master "Core Components" source and release <platform> tags   #
#       (dependent upon <architecture> tags)                          #
#######################################################################
PLATFORM = $(OBJDIR_NAME)

#######################################################################
# [5.0] Master "Core Components" release <tree> tags                  #
#       (dependent upon <architecture> tags)                          #
#######################################################################
ifndef MK_TREE
include $(CORE_DEPTH)/coreconf/tree.mk
endif

#######################################################################
# [6.0] Master "Core Components" source and release <component> tags  #
#       NOTE:  A component is also called a module or a subsystem.    #
#       (dependent upon $(MODULE) being defined on the                #
#        command line, as an environment variable, or in individual   #
#        makefiles, or more appropriately, manifest.mn)               #
#######################################################################
ifndef MK_MODULE
include $(CORE_DEPTH)/coreconf/module.mk
endif

#######################################################################
# [7.0] Master "Core Components" release <version> tags               #
#       (dependent upon $(MODULE) being defined on the                #
#        command line, as an environment variable, or in individual   #
#        makefiles, or more appropriately, manifest.mn)               #
#######################################################################
ifndef MK_VERSION
include $(CORE_DEPTH)/coreconf/version.mk
endif

#######################################################################
# [8.0] Master "Core Components" macros to figure out                 #
#       binary code location                                          #
#       (dependent upon <platform> tags)                              #
#######################################################################
ifndef MK_LOCATION
include $(CORE_DEPTH)/coreconf/location.mk
endif

#######################################################################
# [9.0] Master "Core Components" <component>-specific source path     #
#       (dependent upon <user_source_tree>, <source_component>,       #
#        <version>, and <platform> tags)                              #
#######################################################################
ifndef MK_SOURCE
include $(CORE_DEPTH)/coreconf/source.mk
endif

#######################################################################
# [10.0] Master "Core Components" include switch for support header   #
#        files                                                        #
#        (dependent upon <tree>, <component>, <version>,              #
#         and <platform> tags)                                        #
#######################################################################
ifndef MK_HEADERS
include $(CORE_DEPTH)/coreconf/headers.mk
endif

#######################################################################
# [11.0] Master "Core Components" for computing program prefixes      #
#######################################################################
ifndef MK_PREFIX
include $(CORE_DEPTH)/coreconf/prefix.mk
endif

#######################################################################
# [12.0] Master "Core Components" for computing program suffixes      #
#        (dependent upon <architecture> tags)                         #
#######################################################################
ifndef MK_SUFFIX
include $(CORE_DEPTH)/coreconf/suffix.mk
endif

#######################################################################
# [13.0] Master "Core Components" for defining JDK                    #
#        (dependent upon <architecture>, <source>, and <suffix>  tags)#
#######################################################################
ifdef NS_USE_JDK
include $(CORE_DEPTH)/coreconf/jdk.mk
endif

#######################################################################
# [14.0] Master "Core Components" rule set                            #
#######################################################################
ifndef MK_RULESET
include $(CORE_DEPTH)/coreconf/ruleset.mk
endif

#######################################################################
# [15.0] Dependencies.
#######################################################################

-include $(MKDEPENDENCIES)

