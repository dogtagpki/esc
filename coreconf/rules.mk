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

### where to find NSS if no local build is available
ifeq ($(NSSDIR),)
IMPORT_NSS=1
NSSDIR:=$(CORE_DEPTH)/dist
endif

ifeq ($(USE_NSS),1)
INCLUDES += -I$(NSSDIR)/$(PLATFORM)/include
INCLUDES += -I$(NSSDIR)/public/nss
endif

#######################################################################
###                                                                 ###
###              R U L E S   O F   E N G A G E M E N T              ###
###                                                                 ###
#######################################################################

all:: export libs 

ifeq ($(AUTOCLEAN),1)
autobuild:: clean export private_export libs program install
else
autobuild:: export private_export libs program install
endif

platform::
	@echo $(OBJDIR_NAME)

ifeq (,$(filter-out _WIN%,$(NS_USE_GCC)_$(OS_TARGET)))
USE_NT_C_SYNTAX=1
endif

ifdef XP_OS2_VACPP
USE_NT_C_SYNTAX=1
endif

#
# IMPORTS will always be associated with a component.  Therefore,
# the "import" rule will always change directory to the top-level
# of a component, and traverse the IMPORTS keyword from the
# "manifest.mn" file located at this level only.
#
# note: if there is a trailing slash, the component will be appended
#       (see import.pl - only used for xpheader.jar)

import::
	@echo "== import.pl =="
	@perl -I$(CORE_DEPTH)/coreconf $(CORE_DEPTH)/coreconf/import.pl \
		"RELEASE_TREE=$(RELEASE_TREE)"   \
		"IMPORTS=$(IMPORTS)"             \
		"VERSION=$(VERSION)" \
		"OS_ARCH=$(OS_ARCH)"             \
		"PLATFORM=$(PLATFORM)" \
		"OVERRIDE_IMPORT_CHECK=$(OVERRIDE_IMPORT_CHECK)"   \
		"ALLOW_VERSION_OVERRIDE=$(ALLOW_VERSION_OVERRIDE)" \
		"SOURCE_RELEASE_PREFIX=$(SOURCE_RELEASE_XP_DIR)"   \
		"SOURCE_MD_DIR=$(SOURCE_MD_DIR)"      \
		"SOURCE_XP_DIR=$(SOURCE_XP_DIR)"      \
		"FILES=$(IMPORT_XPCLASS_JAR) $(XPHEADER_JAR) $(MDHEADER_JAR) $(MDBINARY_JAR)" \
		"$(IMPORT_XPCLASS_JAR)=$(IMPORT_XP_DIR)|$(IMPORT_XPCLASS_DIR)|"    \
		"$(XPHEADER_JAR)=$(IMPORT_XP_DIR)|$(SOURCE_XP_DIR)/public/|v" \
		"$(MDHEADER_JAR)=$(IMPORT_MD_DIR)|$(SOURCE_MD_DIR)/include|"        \
		"$(MDBINARY_JAR)=$(IMPORT_MD_DIR)|$(SOURCE_MD_DIR)|"
# On Mac OS X ranlib needs to be rerun after static libs are moved.
ifeq ($(OS_TARGET),Darwin)
	find $(SOURCE_MD_DIR)/lib -name "*.a" -exec $(RANLIB) {} \;
endif

export:: 
	+$(LOOP_OVER_DIRS)

private_export::
	+$(LOOP_OVER_DIRS)

libs program install:: $(TARGETS)
ifdef LIBRARY
	$(INSTALL) -m 664 $(LIBRARY) $(SOURCE_LIB_DIR)
endif
ifdef SHARED_LIBRARY
	$(INSTALL) -m 775 $(SHARED_LIBRARY) $(SOURCE_LIB_DIR)
endif
ifdef IMPORT_LIBRARY
	$(INSTALL) -m 775 $(IMPORT_LIBRARY) $(SOURCE_LIB_DIR)
endif
ifdef PROGRAM
	$(INSTALL) -m 775 $(PROGRAM) $(SOURCE_BIN_DIR)
endif
ifdef PROGRAMS
	$(INSTALL) -m 775 $(PROGRAMS) $(SOURCE_BIN_DIR)
endif
ifdef SCRIPTS
	$(INSTALL) -m 775 $(SCRIPTS) $(SOURCE_BIN_DIR)
endif
	+$(LOOP_OVER_DIRS)

tests::
	+$(LOOP_OVER_DIRS)

clean clobber::
	rm -rf $(ALL_TRASH)
	+$(LOOP_OVER_DIRS)

realclean clobber_all::
	rm -rf $(wildcard *.OBJ) dist $(ALL_TRASH)
	+$(LOOP_OVER_DIRS)

#ifdef ALL_PLATFORMS
#all_platforms:: $(NFSPWD)
#	@d=`$(NFSPWD)`;							\
#	if test ! -d LOGS; then rm -rf LOGS; mkdir LOGS; fi;		\
#	for h in $(PLATFORM_HOSTS); do					\
#		echo "On $$h: $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log";\
#		rsh $$h -n "(chdir $$d;					\
#			     $(MAKE) $(ALL_PLATFORMS) >& LOGS/$$h.log;	\
#			     echo DONE) &" 2>&1 > LOGS/$$h.pid &	\
#		sleep 1;						\
#	done
#
#$(NFSPWD):
#	cd $(@D); $(MAKE) $(@F)
#endif


alltags:
	rm -f TAGS
	find . -name dist -prune -o \( -name '*.[hc]' -o -name '*.cp' -o -name '*.cpp' \) -print | xargs etags -a
	find . -name dist -prune -o \( -name '*.[hc]' -o -name '*.cp' -o -name '*.cpp' \) -print | xargs ctags -a

ifdef XP_OS2_VACPP
# list of libs (such as -lnspr4) do not work for our compiler
# change it to be $(DIST)/lib/nspr4.lib
EXTRA_SHARED_LIBS := $(filter-out -L%,$(EXTRA_SHARED_LIBS))
EXTRA_SHARED_LIBS := $(patsubst -l%,$(DIST)/lib/%.$(LIB_SUFFIX),$(EXTRA_SHARED_LIBS))
endif

$(PROGRAM): $(OBJS) $(EXTRA_LIBS)
	@$(MAKE_OBJDIR)
ifeq (,$(filter-out _WIN%,$(NS_USE_GCC)_$(OS_TARGET)))
	$(MKPROG) $(subst /,\\,$(OBJS)) -Fe$@ -link $(LDFLAGS) $(subst /,\\,$(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS))
else
ifdef XP_OS2_VACPP
	$(MKPROG) -Fe$@ $(CFLAGS) $(OBJS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS)
else
	$(MKPROG) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS)
endif
endif

get_objs:
	@echo $(OBJS)

$(LIBRARY): $(OBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
ifeq (,$(filter-out WIN%,$(OS_TARGET)))
	$(AR) $(subst /,\\,$(OBJS))
else
	$(AR) $(OBJS)
endif
	$(RANLIB) $@


ifeq ($(OS_TARGET),OS2)
$(IMPORT_LIBRARY): $(MAPFILE)
	rm -f $@
	$(IMPLIB) $@ $(MAPFILE)
	$(RANLIB) $@
endif

ifdef SHARED_LIBRARY_LIBS
ifdef BUILD_TREE
SUB_SHLOBJS = $(foreach dir,$(SHARED_LIBRARY_DIRS),$(shell $(MAKE) -C $(dir) --no-print-directory get_objs))
else
SUB_SHLOBJS = $(foreach dir,$(SHARED_LIBRARY_DIRS),$(addprefix $(dir)/,$(shell $(MAKE) -C $(dir) --no-print-directory get_objs)))
endif
endif

$(SHARED_LIBRARY): $(OBJS) $(RES) $(MAPFILE) $(SUB_SHLOBJS)
	@$(MAKE_OBJDIR)
	rm -f $@
ifeq ($(OS_TARGET)$(OS_RELEASE), AIX4.1)
	echo "#!" > $(OBJDIR)/lib$(LIBRARY_NAME)_syms
	nm -B -C -g $(OBJS) \
	| awk '/ [T,D] / {print $$3}' \
	| sed -e 's/^\.//' \
	| sort -u >> $(OBJDIR)/lib$(LIBRARY_NAME)_syms
	$(LD) $(XCFLAGS) -o $@ $(OBJS) -bE:$(OBJDIR)/lib$(LIBRARY_NAME)_syms \
	-bM:SRE -bnoentry $(OS_LIBS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS)
else
ifeq (,$(filter-out WIN%,$(OS_TARGET)))
ifdef NS_USE_GCC
	$(LINK_DLL) $(OBJS) $(SUB_SHLOBJS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS) $(LD_LIBS) $(RES)
else
	$(LINK_DLL) -MAP $(DLLBASE) $(subst /,\\,$(OBJS) $(SUB_SHLOBJS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS) $(LD_LIBS) $(RES))
endif
else
ifdef XP_OS2_VACPP
	$(MKSHLIB) $(DLLFLAGS) $(LDFLAGS) $(OBJS) $(SUB_SHLOBJS) $(LD_LIBS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS)
else
	$(MKSHLIB) -o $@ $(OBJS) $(SUB_SHLOBJS) $(LD_LIBS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS)
endif
	chmod +x $@
ifeq ($(OS_TARGET),Darwin)
ifdef MAPFILE
	nmedit -s $(MAPFILE) $@
endif
endif
endif
endif

ifeq (,$(filter-out WIN%,$(OS_TARGET)))
$(RES): $(RESNAME)
	@$(MAKE_OBJDIR)
# The resource compiler does not understand the -U option.
ifdef NS_USE_GCC
	$(RC) $(filter-out -U%,$(DEFINES)) $(INCLUDES:-I%=--include-dir %) -o $@ $<
else
	$(RC) $(filter-out -U%,$(DEFINES)) $(INCLUDES) -Fo$@ $<
endif
	@echo $(RES) finished
endif

$(MAPFILE): $(LIBRARY_NAME).def
	@$(MAKE_OBJDIR)
	$(PROCESS_MAP_FILE)


$(OBJDIR)/$(PROG_PREFIX)%$(PROG_SUFFIX): $(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX)
	@$(MAKE_OBJDIR)
ifeq (,$(filter-out _WIN%,$(NS_USE_GCC)_$(OS_TARGET)))
	$(MKPROG) $(OBJDIR)/$(PROG_PREFIX)$*$(OBJ_SUFFIX) -Fe$@ -link \
	$(LDFLAGS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS)
else
	$(MKPROG) -o $@ $(OBJDIR)/$(PROG_PREFIX)$*$(OBJ_SUFFIX) \
	$(LDFLAGS) $(EXTRA_LIBS) $(EXTRA_SHARED_LIBS) $(OS_LIBS)
endif

WCCFLAGS1 := $(subst /,\\,$(CFLAGS))
WCCFLAGS2 := $(subst -I,-i=,$(WCCFLAGS1))
WCCFLAGS3 := $(subst -D,-d,$(WCCFLAGS2))

# Translate source filenames to absolute paths. This is required for
# debuggers under Windows & OS/2 to find source files automatically

ifeq (,$(filter-out OS2%,$(OS_TARGET)))
NEED_ABSOLUTE_PATH := 1
PWD := $(shell pwd)
endif

ifeq (,$(filter-out _WIN%,$(NS_USE_GCC)_$(OS_TARGET)))
NEED_ABSOLUTE_PATH := 1
ifeq (,$(findstring ;,$(PATH)))
PWD :=  $(subst \,/,$(shell cygpath -w `pwd`))
else
PWD := $(shell pwd)
endif
endif

ifdef NEED_ABSOLUTE_PATH
abspath = $(if $(findstring :,$(1)),$(1),$(if $(filter /%,$(1)),$(1),$(PWD)/$(1)))
else
abspath = $(1)
endif

$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.c
	@$(MAKE_OBJDIR)
ifdef USE_NT_C_SYNTAX
	$(CC) -Fo$@ -c $(CFLAGS) $(call abspath,$<)
else
ifdef NEED_ABSOLUTE_PATH
	$(CC) -o $@ -c $(CFLAGS) $(call abspath,$<)
else
	$(CC) -o $@ -c $(CFLAGS) $<
endif
endif

$(PROG_PREFIX)%$(OBJ_SUFFIX): %.c
ifdef USE_NT_C_SYNTAX
	$(CC) -Fo$@ -c $(CFLAGS) $(call abspath,$<)
else
ifdef NEED_ABSOLUTE_PATH
	$(CC) -o $@ -c $(CFLAGS) $(call abspath,$<)
else
	$(CC) -o $@ -c $(CFLAGS) $<
endif
endif

ifndef XP_OS2_VACPP
ifneq (,$(filter-out _WIN%,$(NS_USE_GCC)_$(OS_TARGET)))
$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.s
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $<
endif
endif

$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.asm
	@$(MAKE_OBJDIR)
ifdef XP_OS2_VACPP
	$(AS) -Fdo:$(OBJDIR) $(ASFLAGS) $(subst /,\\,$<)
else
	$(AS) -Fo$@ $(ASFLAGS) -c $<
endif

$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.S
	@$(MAKE_OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $<

$(OBJDIR)/$(PROG_PREFIX)%: %.cpp
	@$(MAKE_OBJDIR)
ifdef USE_NT_C_SYNTAX
	$(CCC) -Fo$@ -c $(CFLAGS) $(call abspath,$<)
else
ifdef NEED_ABSOLUTE_PATH
	$(CCC) -o $@ -c $(CFLAGS) $(call abspath,$<)
else
	$(CCC) -o $@ -c $(CFLAGS) $<
endif
endif

#
# Please keep the next two rules in sync.
#
$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.cc
	@$(MAKE_OBJDIR)
	$(CCC) -o $@ -c $(CFLAGS) $<

$(OBJDIR)/$(PROG_PREFIX)%$(OBJ_SUFFIX): %.cpp
	@$(MAKE_OBJDIR)
ifdef STRICT_CPLUSPLUS_SUFFIX
	echo "#line 1 \"$<\"" | cat - $< > $(OBJDIR)/t_$*.cc
	$(CCC) -o $@ -c $(CFLAGS) $(OBJDIR)/t_$*.cc
	rm -f $(OBJDIR)/t_$*.cc
else
ifdef USE_NT_C_SYNTAX
	$(CCC) -Fo$@ -c $(CFLAGS) $(call abspath,$<)
else
ifdef NEED_ABSOLUTE_PATH
	$(CCC) -o $@ -c $(CFLAGS) $(call abspath,$<)
else
	$(CCC) -o $@ -c $(CFLAGS) $<
endif
endif
endif #STRICT_CPLUSPLUS_SUFFIX

%.i: %.cpp
ifeq (,$(filter-out WIN%,$(OS_TARGET)))
	$(CCC) -C /P $(CFLAGS) $< 
else
	$(CCC) -C -E $(CFLAGS) $< > $*.i
endif

%.i: %.c
ifeq (,$(filter-out WIN%,$(OS_TARGET)))
	$(CC) -C /P $(CFLAGS) $< 
else
	$(CC) -C -E $(CFLAGS) $< > $*.i
endif

ifneq (,$(filter-out WIN%,$(OS_TARGET)))
%.i: %.s
	$(CC) -C -E $(CFLAGS) $< > $*.i
endif

%: %.pl
	rm -f $@; cp $*.pl $@; chmod +x $@

$(OBJDIR)/$(PROG_PREFIX)%: %.sh
	rm -f $@; cp $*.sh $@; chmod +x $@

%: %.sh
	rm -f $@; cp $*.sh $@; chmod +x $@

ifdef DIRS
$(DIRS)::
	@if test -d $@; then				\
		set $(EXIT_ON_ERROR);			\
		echo "cd $@; $(MAKE)";			\
		cd $@; $(MAKE);				\
		set +e;					\
	else						\
		echo "Skipping non-directory $@...";	\
	fi;						\
	$(CLICK_STOPWATCH)
endif

################################################################################
# Bunch of things that extend the 'export' rule (in order):
################################################################################

$(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE) $(JMCSRCDIR)::
	@if test ! -d $@; then	    \
		echo Creating $@;   \
		rm -rf $@;	    \
		$(NSINSTALL) -D $@; \
	fi

################################################################################
## IDL_GEN

ifneq ($(IDL_GEN),)

#export::
#	$(IDL2JAVA) $(IDL_GEN)

#all:: export

#clobber::
#	rm -f $(IDL_GEN:.idl=.class)	# XXX wrong!

endif

################################################################################
### JSRCS -- for compiling java files
###
###          NOTE:  For backwards compatibility, if $(NETLIBDEPTH) is defined,
###                 replace $(CORE_DEPTH) with $(NETLIBDEPTH).
###

ifneq ($(JSRCS),)
ifneq ($(JAVAC),)
ifdef NETLIBDEPTH
	CORE_DEPTH := $(NETLIBDEPTH)
endif

JAVA_EXPORT_SRCS=$(shell perl $(CORE_DEPTH)/coreconf/outofdate.pl $(PERLARG)	-d $(JAVA_DESTPATH)/$(PACKAGE) $(JSRCS) $(PRIVATE_JSRCS))

export:: $(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE)
ifneq ($(JAVA_EXPORT_SRCS),)
	$(JAVAC) $(JAVA_EXPORT_SRCS)
endif

all:: export

clobber::
	rm -f $(SOURCE_XP_DIR)/classes/$(PACKAGE)/*.class

endif
endif

#
# JDIRS -- like JSRCS, except you can give a list of directories and it will
# compile all the out-of-date java files in those directories.
#
# NOTE: recursing through these can speed things up, but they also cause
# some builds to run out of memory
#
# NOTE:  For backwards compatibility, if $(NETLIBDEPTH) is defined,
#        replace $(CORE_DEPTH) with $(NETLIBDEPTH).
#
ifdef JDIRS
ifneq ($(JAVAC),)
ifdef NETLIBDEPTH
	CORE_DEPTH := $(NETLIBDEPTH)
endif

# !!!!! THIS WILL CRASH SHMSDOS.EXE !!!!!
# shmsdos does not support shell variables. It will crash when it tries
# to parse the '=' character. A solution is to rewrite outofdate.pl so it
# takes the Javac command as an argument and executes the command itself,
# instead of returning a list of files.
export:: $(JAVA_DESTPATH) $(JAVA_DESTPATH)/$(PACKAGE)
	@echo "!!! THIS COMMAND IS BROKEN ON WINDOWS--SEE rules.mk FOR DETAILS !!!"
	return -1
	@for d in $(JDIRS); do							\
		if test -d $$d; then						\
			set $(EXIT_ON_ERROR);					\
			files=`echo $$d/*.java`;				\
			list=`perl $(CORE_DEPTH)/coreconf/outofdate.pl $(PERLARG)	\
				    -d $(JAVA_DESTPATH)/$(PACKAGE) $$files`;	\
			if test "$${list}x" != "x"; then			\
			    echo Building all java files in $$d;		\
			    echo $(JAVAC) $$list;				\
			    $(JAVAC) $$list;					\
			fi;							\
			set +e;							\
		else								\
			echo "Skipping non-directory $$d...";			\
		fi;								\
		$(CLICK_STOPWATCH);						\
	done
endif
endif

#
# JDK_GEN -- for generating "old style" native methods 
#
# Generate JDK Headers and Stubs into the '_gen' and '_stubs' directory
#
# NOTE:  For backwards compatibility, if $(NETLIBDEPTH) is defined,
#        replace $(CORE_DEPTH) with $(NETLIBDEPTH).
#
ifneq ($(JDK_GEN),)
ifneq ($(JAVAH),)
ifdef NSBUILDROOT
	INCLUDES += -I$(JDK_GEN_DIR) -I$(SOURCE_XP_DIR)
else
	INCLUDES += -I$(JDK_GEN_DIR)
endif

ifdef NETLIBDEPTH
	CORE_DEPTH := $(NETLIBDEPTH)
endif

JDK_PACKAGE_CLASSES	:= $(JDK_GEN)
JDK_PATH_CLASSES	:= $(subst .,/,$(JDK_PACKAGE_CLASSES))
JDK_HEADER_CLASSFILES	:= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JDK_PATH_CLASSES))
JDK_STUB_CLASSFILES	:= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JDK_PATH_CLASSES))
JDK_HEADER_CFILES	:= $(patsubst %,$(JDK_GEN_DIR)/%.h,$(JDK_GEN))
JDK_STUB_CFILES		:= $(patsubst %,$(JDK_STUB_DIR)/%.c,$(JDK_GEN))

$(JDK_HEADER_CFILES): $(JDK_HEADER_CLASSFILES)
$(JDK_STUB_CFILES): $(JDK_STUB_CLASSFILES)

export::
	@echo Generating/Updating JDK headers 
	$(JAVAH) -d $(JDK_GEN_DIR) $(JDK_PACKAGE_CLASSES)
	@echo Generating/Updating JDK stubs
	$(JAVAH) -stubs -d $(JDK_STUB_DIR) $(JDK_PACKAGE_CLASSES)
ifndef NO_MAC_JAVA_SHIT
	@if test ! -d $(CORE_DEPTH)/lib/mac/Java/; then						\
		echo "!!! You need to have a ns/lib/mac/Java directory checked out.";		\
		echo "!!! This allows us to automatically update generated files for the mac.";	\
		echo "!!! If you see any modified files there, please check them in.";		\
	fi
	@echo Generating/Updating JDK headers for the Mac
	$(JAVAH) -mac -d $(CORE_DEPTH)/lib/mac/Java/_gen $(JDK_PACKAGE_CLASSES)
	@echo Generating/Updating JDK stubs for the Mac
	$(JAVAH) -mac -stubs -d $(CORE_DEPTH)/lib/mac/Java/_stubs $(JDK_PACKAGE_CLASSES)
endif
endif
endif

#
# JRI_GEN -- for generating "old style" JRI native methods
#
# Generate JRI Headers and Stubs into the 'jri' directory
#
# NOTE:  For backwards compatibility, if $(NETLIBDEPTH) is defined,
#        replace $(CORE_DEPTH) with $(NETLIBDEPTH).
#
ifneq ($(JRI_GEN),)
ifneq ($(JAVAH),)
ifdef NSBUILDROOT
	INCLUDES += -I$(JRI_GEN_DIR) -I$(SOURCE_XP_DIR)
else
	INCLUDES += -I$(JRI_GEN_DIR)
endif

ifdef NETLIBDEPTH
	CORE_DEPTH := $(NETLIBDEPTH)
endif

JRI_PACKAGE_CLASSES	:= $(JRI_GEN)
JRI_PATH_CLASSES	:= $(subst .,/,$(JRI_PACKAGE_CLASSES))
JRI_HEADER_CLASSFILES	:= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JRI_PATH_CLASSES))
JRI_STUB_CLASSFILES	:= $(patsubst %,$(JAVA_DESTPATH)/%.class,$(JRI_PATH_CLASSES))
JRI_HEADER_CFILES	:= $(patsubst %,$(JRI_GEN_DIR)/%.h,$(JRI_GEN))
JRI_STUB_CFILES		:= $(patsubst %,$(JRI_GEN_DIR)/%.c,$(JRI_GEN))

$(JRI_HEADER_CFILES): $(JRI_HEADER_CLASSFILES)
$(JRI_STUB_CFILES): $(JRI_STUB_CLASSFILES)

export::
	@echo Generating/Updating JRI headers 
	$(JAVAH) -jri -d $(JRI_GEN_DIR) $(JRI_PACKAGE_CLASSES)
	@echo Generating/Updating JRI stubs
	$(JAVAH) -jri -stubs -d $(JRI_GEN_DIR) $(JRI_PACKAGE_CLASSES)
ifndef NO_MAC_JAVA_SHIT
	@if test ! -d $(CORE_DEPTH)/lib/mac/Java/; then						\
		echo "!!! You need to have a ns/lib/mac/Java directory checked out.";		\
		echo "!!! This allows us to automatically update generated files for the mac.";	\
		echo "!!! If you see any modified files there, please check them in.";		\
	fi
	@echo Generating/Updating JRI headers for the Mac
	$(JAVAH) -jri -mac -d $(CORE_DEPTH)/lib/mac/Java/_jri $(JRI_PACKAGE_CLASSES)
	@echo Generating/Updating JRI stubs for the Mac
	$(JAVAH) -jri -mac -stubs -d $(CORE_DEPTH)/lib/mac/Java/_jri $(JRI_PACKAGE_CLASSES)
endif
endif
endif

#
# JNI_GEN -- for generating JNI native methods
#
# Generate JNI Headers into the 'jni' directory
#
ifneq ($(JNI_GEN),)
ifneq ($(JAVAH),)
JNI_HEADERS		:= $(patsubst %,$(JNI_GEN_DIR)/%.h,$(JNI_GEN))

export::
	@if test ! -d $(JNI_GEN_DIR); then						\
		echo $(JAVAH) -jni -d $(JNI_GEN_DIR) $(JNI_GEN);			\
		$(JAVAH) -jni -d $(JNI_GEN_DIR) $(JNI_GEN);				\
	else										\
		echo "Checking for out of date header files" ;                          \
		perl $(CORE_DEPTH)/coreconf/jniregen.pl $(PERLARG)			\
		 -d $(JAVA_DESTPATH) -j "$(JAVAH) -jni -d $(JNI_GEN_DIR)" $(JNI_GEN);\
	fi
endif
endif

#
# JMC_EXPORT -- for declaring which java classes are to be exported for jmc
#
ifneq ($(JMC_EXPORT),)
JMC_EXPORT_PATHS	:= $(subst .,/,$(JMC_EXPORT))
JMC_EXPORT_FILES	:= $(patsubst %,$(JAVA_DESTPATH)/$(PACKAGE)/%.class,$(JMC_EXPORT_PATHS))

#
# We're doing NSINSTALL -t here (copy mode) because calling INSTALL will pick up 
# your NSDISTMODE and make links relative to the current directory. This is a
# problem because the source isn't in the current directory:
#
export:: $(JMC_EXPORT_FILES) $(JMCSRCDIR)
	$(NSINSTALL) -t -m 444 $(JMC_EXPORT_FILES) $(JMCSRCDIR)
endif

#
# JMC_GEN -- for generating java modules
#
# Provide default export & install rules when using JMC_GEN
#
ifneq ($(JMC_GEN),)
ifneq ($(JMC),)
	INCLUDES    += -I$(JMC_GEN_DIR) -I.
	JMC_HEADERS := $(patsubst %,$(JMC_GEN_DIR)/%.h,$(JMC_GEN))
	JMC_STUBS   := $(patsubst %,$(JMC_GEN_DIR)/%.c,$(JMC_GEN))
	JMC_OBJS    := $(patsubst %,$(OBJDIR)/%$(OBJ_SUFFIX),$(JMC_GEN))

$(JMC_GEN_DIR)/M%.h: $(JMCSRCDIR)/%.class
	$(JMC) -d $(JMC_GEN_DIR) -interface $(JMC_GEN_FLAGS) $(?F:.class=)

$(JMC_GEN_DIR)/M%.c: $(JMCSRCDIR)/%.class
	$(JMC) -d $(JMC_GEN_DIR) -module $(JMC_GEN_FLAGS) $(?F:.class=)

$(OBJDIR)/M%$(OBJ_SUFFIX): $(JMC_GEN_DIR)/M%.h $(JMC_GEN_DIR)/M%.c
	@$(MAKE_OBJDIR)
	$(CC) -o $@ -c $(CFLAGS) $(JMC_GEN_DIR)/M$*.c

export:: $(JMC_HEADERS) $(JMC_STUBS)
endif
endif

#
# Copy each element of EXPORTS to $(SOURCE_XP_DIR)/public/$(MODULE)/
#
PUBLIC_EXPORT_DIR = $(SOURCE_XP_DIR)/public/$(MODULE)

ifneq ($(EXPORTS),)
$(PUBLIC_EXPORT_DIR)::
	@if test ! -d $@; then	    \
		echo Creating $@;   \
		$(NSINSTALL) -D $@; \
	fi

export:: $(PUBLIC_EXPORT_DIR) 

export:: $(EXPORTS) 
	$(INSTALL) -m 444 $^ $(PUBLIC_EXPORT_DIR)

export:: $(BUILT_SRCS)
endif

# Duplicate export rule for private exports, with different directories

PRIVATE_EXPORT_DIR = $(SOURCE_XP_DIR)/private/$(MODULE)

ifneq ($(PRIVATE_EXPORTS),)
$(PRIVATE_EXPORT_DIR)::
	@if test ! -d $@; then	    \
		echo Creating $@;   \
		$(NSINSTALL) -D $@; \
	fi

private_export:: $(PRIVATE_EXPORT_DIR)

private_export:: $(PRIVATE_EXPORTS) 
	$(INSTALL) -m 444 $^ $(PRIVATE_EXPORT_DIR)
else
private_export:: 
	@echo There are no private exports.;
endif

##########################################################################
###   RULES FOR RUNNING REGRESSION SUITE TESTS
###   REQUIRES 'REGRESSION_SPEC' TO BE SET TO THE NAME OF A REGRESSION SPECFILE
###   AND RESULTS_SUBDIR TO BE SET TO SOMETHING LIKE SECURITY/PKCS5
##########################################################################

TESTS_DIR = $(RESULTS_DIR)/$(RESULTS_SUBDIR)/$(OS_TARGET)$(OS_RELEASE)$(CPU_TAG)$(COMPILER_TAG)$(IMPL_STRATEGY)

ifneq ($(REGRESSION_SPEC),)

ifneq ($(BUILD_OPT),)
REGDATE = $(subst \ ,, $(shell perl  $(CORE_DEPTH)/$(MODULE)/scripts/now))
endif

tests:: $(REGRESSION_SPEC) 
	cd $(PLATFORM); \
	../$(SOURCE_MD_DIR)/bin/regress$(PROG_SUFFIX) specfile=../$(REGRESSION_SPEC) progress $(EXTRA_REGRESS_OPTIONS); \
	if test ! -d $(TESTS_DIR); then \
		echo Creating $(TESTS_DIR);   \
		$(NSINSTALL) -D $(TESTS_DIR); \
	fi
ifneq ($(BUILD_OPT),)
	$(NSINSTALL) -m 664 $(PLATFORM)/$(REGDATE).sum $(TESTS_DIR); \
	$(NSINSTALL) -m 664 $(PLATFORM)/$(REGDATE).htm $(TESTS_DIR); \
	echo "Please now make sure your results files are copied to $(TESTS_DIR), "; \
	echo "then run 'reporter specfile=$(RESULTS_DIR)/rptspec'"
endif
else
tests:: 
	@echo Error: you didn't specify REGRESSION_SPEC in your manifest.mn file!;
endif


# Duplicate export rule for releases, with different directories

ifneq ($(EXPORTS),)
$(SOURCE_RELEASE_XP_DIR)/include::
	@if test ! -d $@; then	    \
		echo Creating $@;   \
		$(NSINSTALL) -D $@; \
	fi

endif




################################################################################

-include $(DEPENDENCIES)

ifneq (,$(filter-out Linux OpenVMS OS2 WIN%,$(OS_TARGET)))
# Can't use sed because of its 4000-char line length limit, so resort to perl
.DEFAULT:
	@perl -e '                                                            \
	    open(MD, "< $(DEPENDENCIES)");                                    \
	    while (<MD>) {                                                    \
		if (m@ \.*/*$< @) {                                           \
		    $$found = 1;                                              \
		    last;                                                     \
		}                                                             \
	    }                                                                 \
	    if ($$found) {                                                    \
		print "Removing stale dependency $< from $(DEPENDENCIES)\n";  \
		seek(MD, 0, 0);                                               \
		$$tmpname = "$(OBJDIR)/fix.md" . $$$$;                        \
		open(TMD, "> " . $$tmpname);                                  \
		while (<MD>) {                                                \
		    s@ \.*/*$< @ @;                                           \
		    if (!print TMD "$$_") {                                   \
			unlink(($$tmpname));                                  \
			exit(1);                                              \
		    }                                                         \
		}                                                             \
		close(TMD);                                                   \
		if (!rename($$tmpname, "$(DEPENDENCIES)")) {                  \
		    unlink(($$tmpname));                                      \
		}                                                             \
	    } elsif ("$<" ne "$(DEPENDENCIES)") {                             \
		print "$(MAKE): *** No rule to make target $<.  Stop.\n";     \
		exit(1);                                                      \
	    }'
endif

#############################################################################
# X dependency system
#############################################################################

ifdef MKDEPENDENCIES

# For Windows, $(MKDEPENDENCIES) must be -included before including rules.mk

$(MKDEPENDENCIES)::
	@$(MAKE_OBJDIR)
	touch $(MKDEPENDENCIES) 
	chmod u+w $(MKDEPENDENCIES) 
#on NT, the preceeding touch command creates a read-only file !?!?!
#which is why we have to explicitly chmod it.
	$(MKDEPEND) -p$(OBJDIR_NAME)/ -o'$(OBJ_SUFFIX)' -f$(MKDEPENDENCIES) \
$(NOMD_CFLAGS) $(YOPT) $(CSRCS) $(CPPSRCS) $(ASFILES)

$(MKDEPEND):: $(MKDEPEND_DIR)/*.c $(MKDEPEND_DIR)/*.h
	cd $(MKDEPEND_DIR); $(MAKE)

ifdef OBJS
depend:: $(MKDEPEND) $(MKDEPENDENCIES)
else
depend::
endif
	+$(LOOP_OVER_DIRS)

dependclean::
	rm -f $(MKDEPENDENCIES)
	+$(LOOP_OVER_DIRS)

#-include $(NSINSTALL_DIR)/$(OBJDIR)/depend.mk

else
depend::
endif

################################################################################
# Special gmake rules.
################################################################################

#
# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.
#
.SUFFIXES:
.SUFFIXES: .out .a .ln .o .obj .c .cc .C .cpp .y .l .s .S .h .sh .i .pl .class .java .html .asm

#
# Don't delete these files if we get killed.
#
.PRECIOUS: .java $(JDK_HEADERS) $(JDK_STUBS) $(JRI_HEADERS) $(JRI_STUBS) $(JMC_HEADERS) $(JMC_STUBS) $(JNI_HEADERS)

#
# Fake targets.  Always run these rules, even if a file/directory with that
# name already exists.
#
.PHONY: all all_platforms alltags boot clean clobber clobber_all export install libs program realclean $(OBJDIR) $(DIRS)

