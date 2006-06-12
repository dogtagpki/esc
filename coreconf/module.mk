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
# The master "Core Components" source and release component directory #
# names are ALWAYS identical and are the value of $(MODULE).          #
# NOTE:  A component is also called a module or a subsystem.          #
#######################################################################

#
#  All "Core Components" <component>-specific source-side tags must
#  always be identified for compiling/linking purposes
#

ifndef JAVA_SOURCE_COMPONENT
    JAVA_SOURCE_COMPONENT = java
endif

ifndef NETLIB_SOURCE_COMPONENT
    NETLIB_SOURCE_COMPONENT = netlib
endif

ifndef NSPR_SOURCE_COMPONENT
    NSPR_SOURCE_COMPONENT = nspr20
endif

ifndef SECTOOLS_SOURCE_COMPONENT
    SECTOOLS_SOURCE_COMPONENT = sectools
endif

ifndef SECURITY_SOURCE_COMPONENT
    SECURITY_SOURCE_COMPONENT = security
endif

MK_MODULE = included
