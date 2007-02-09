#! gmake
#
# BEGIN COPYRIGHT BLOCK
# This Program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 2 of the License.
#
# This Program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.#
# You should have received a copy of the GNU General Public License along with
# this Program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA.
#
# Copyright (C) 2005 Red Hat, Inc.
# All rights reserved.
# END COPYRIGHT BLOCK

CORE_DEPTH = ../..
include $(CORE_DEPTH)/coreconf/config.mk
include $(CORE_DEPTH)/coreconf/rules.mk

#This just builds Xulrunner if necessary

TYPE=Debug
ifdef BUILD_OPT
TYPE=Release
endif

MOZ_SRCDIR=$(SOURCE_XP_DIR)/src/mozilla
MOZ_OBJDIR=$(DIST)/xulrunner_build
MOZ_RELOBJDIR=$(shell pwd)/$(MOZ_OBJDIR)

default: $(MOZ_OBJDIR)

#use the dependencies to know if we need to rebuild or not
export::  $(MOZ_OBJDIR)
lib::  $(MOZ_OBJDIR)
install::  $(MOZ_OBJDIR)

clean::

$(MOZ_OBJDIR): $(MOZ_SRCDIR)/.mozconfig $(MOZ_SRCDIR)/client.mk
	(cd $(MOZ_SRCDIR) ; make -f client.mk build ) 
ifneq ($(OS_ARCH),Darwin)
	(cd $(MOZ_OBJDIR)/xulrunner/installer ; make )
endif
	touch $(MOZ_OBJDIR)

$(MOZ_SRCDIR)/.mozconfig: $(SOURCE_XP_DIR)/target.$(TYPE)
	echo $@
	rm -f $@
	echo "mk_add_options MOZ_CO_PROJECT=xulrunner" > $@.$(TYPE)
	echo "mk_add_options MOZ_OBJDIR=$(MOZ_RELOBJDIR)" >> $@.$(TYPE)
	echo "ac_add_options --enable-application=xulrunner" >> $@.$(TYPE)
	echo "ac_add_options --disable-tests" >> $@.$(TYPE)
	echo "ac_cv_visibility_pragma=no" >> $@.$(TYPE)
ifdef BUILD_OPT
	echo "ac_add_options --enable-optimize" >> $@.$(TYPE)
	echo "ac_add_options --disable-debug" >> $@.$(TYPE)
else
	echo "ac_add_options --disable-optimize" >> $@.$(TYPE)
	echo "ac_add_options --enable-debug" >> $@.$(TYPE)
endif
ifeq ($(OS_ARCH),Linux)
	echo "ac_add_options --disable-javaxpcom" >> $@.$(TYPE)
	echo "ac_add_options --enable-default-toolkit=gtk2" >> $@.$(TYPE)
	echo "ac_add_options --enable-xft" >> $@.$(TYPE)
	echo "ac_add_options --disable-freetype2" >> $@.$(TYPE)
ifndef NO_SYSTEM_NSS 
	echo "ac_add_options --with-system-nspr" >> $@.$(TYPE)
	echo "ac_add_options --with-system-nss" >> $@.$(TYPE)
endif
endif
ifeq ($(OS_ARCH),Darwin)
	echo ". \$$topsrcdir/build/macosx/universal/mozconfig" >> $@.$(TYPE)
	echo "ac_add_options --with-macos-sdk=/Developer/SDKs/MacOSX10.4u.sdk" >> $@.$(TYPE)
endif

ifeq ($(OS_ARCH), WINNT)
	echo "ac_add_options --disable-javaxpcom" >> $@.$(TYPE)
endif
	cp $@.$(TYPE) $@
	cat $@
   
#detect changes in .mozconfig and restart mozilla builds if necessary
# we can only do a debug or a release build at one time. if we
# try to run concurrent builds in the same tree, we will fail.
#NOTE: This also means we can't do multiple builds on different platforms
# at the same time in the mozilla tree
$(SOURCE_XP_DIR)/target.Debug:
	rm -f $(SOURCE_XP_DIR)/target.Release # release is 'out of date' now
	touch $(SOURCE_XP_DIR)/target.Debug

$(SOURCE_XP_DIR)/target.Release:
	rm -f $(SOURCE_XP_DIR)/target.Debug # debug is 'out of date' now
	touch $(SOURCE_XP_DIR)/target.Release


