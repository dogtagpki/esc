#!/bin/bash
# Command line arg values

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

###### Supported enrivonment variables ############

### MSVC_PATH  -  Path to the directory containing usual Microsoft dlls

### INNO_PATH  -  Path name of Inno installer executable

### CSP_PATH   -  Path to the directory containing the CSP if desired

NUM_ARGS=0
ARG_COMMAND=


# NSS values
# NSS needed just to help coolkey build

NSS_NAME=nss-3.11.4
NSS_ARCHIVE=$NSS_NAME-with-nspr-4.6.4
NSS_SOURCE_URL=https://ftp.mozilla.org/pub/mozilla.org/security/nss/releases/NSS_3_11_4_RTM/src/$NSS_ARCHIVE.tar.gz

#Inno installer values

#INNO_PATH="C:/Program Files/Inno Setup 5/ISCC.exe"

#Egate driver values

EGATE_DRIVER_URL=http://www.it-secure.com/Downloads
EGATE_DRIVER_NAME=e-gate_W2k_XP_24.zip

#Zlib values

ZLIB_NAME=zlib
ZLIB_DLL=zlib1

ZLIB_ARCHIVE=zlib123-dll
ZLIB_BIN_URL=http://www.zlib.net

#CoolKey values

COOLKEY_NAME=coolkey
COOLKEY_TAG=HEAD

#Fedora repo for CoolKey and ESC

FEDORA_CVS_ROOT=:pserver:anonymous@cvs.fedora.redhat.com:/cvs/dirsec

#Xulrunner values


XULRUNNER_ARCHIVE_NAME=xulrunner-1.8.0.4-source.tar.bz2
XULRUNNER_SRC_URL=http://ftp.mozilla.org/pub/mozilla.org/xulrunner/releases/1.8.0.4/source/$XULRUNNER_ARCHIVE_NAME

#Base Dirctory calc

BASE_DIR=${PWD}

#ESC values

ESC_NAME=esc
ESC_VERSION_NO=1.1.0-3


#Cygwin values

ORIG_PATH=${PATH}

CYGWIN_BIN_PATH=/cygdrive/c/cygwin/bin

#Value for the location of Moz Tools needed to compile

MOZ_TOOLS_BIN_PATH=/cygdrive/c/moztools/bin:/cygdrive/d/moztools/bin

export PATH=${MOZ_TOOLS_BIN_PATH}:${ORIG_PATH}

CORE_OBJ_DIR=`uname``uname -r`_OPT.OBJ

export PATH=${ORIG_PATH}



function buildNSS  {

    echo "BUILDING NSS..."

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doNSS ];
    then
        echo "Do not build NSS." 
        return 0
    fi


    cd $BASE_DIR

    if [ -d $NSS_NAME ];
    then
        echo "NSS already checked out."
    else

    wget --no-check-certificate $NSS_SOURCE_URL
    if [ ! -f ./$NSS_ARCHIVE.tar.gz ];
    then
       echo "Can't download NSS"
       return 1

    fi

    tar -xzvf $NSS_ARCHIVE.tar.gz
    if [ $? != 0 ];
    then
        echo "Can't untar NSS."
        return 1
    fi
    fi

    cd $NSS_NAME/mozilla/security/nss
    make BUILD_OPT=1 nss_build_all
    if [ $? != 0 ];
    then
        echo "Can't make nss."
        return 1
        
    fi

    return 0

}

function buildCOOLKEY {

   echo "BUILDING COOLKEY..."
   cd $BASE_DIR

   if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doCoolKey ];
   then
       echo "Do not build CoolKey"
       return 0
    fi

   export PATH=$CYGWIN_BIN_PATH:${PATH}

   if [ -d coolkey ];
   then
       echo "Echo CoolKey already checked out." 
   else
       cvs  -d $FEDORA_CVS_ROOT co -r $COOLKEY_TAG coolkey 
   fi

   cd $COOLKEY_NAME

  
   autoconf-2.5x

   if [ $? != 0 ];
   then
       echo "Coolkey autoconf failed.."
       export PATH=${ORIG_PATH}
       return 1
   fi
 
   ZLIB_INC_PATH=${BASE_DIR}/zlib/include
   ZLIB_LIB_PATH=${BASE_DIR}/zlib/lib

   ZLIB_LIB_FLAGS=${BASE_DIR}/zlib/$ZLIB_DLL.dll

   ZLIB_INC_PATH=`cygpath -m $ZLIB_INC_PATH`
   ZLIB_LIB_PATH=`cygpath -m $ZLIB_LIB_PATH`

   ZLIB_LIB_FLAGS=`cygpath -m $ZLIB_LIB_FLAGS` 

   NSS_CFLAGS_BASE_PATH=${BASE_DIR}/$NSS_NAME/mozilla/dist

   NSS_CFLAGS_BASE_PATH=`cygpath -m $NSS_CFLAGS_BASE_PATH`


   NSS_CFLAGS="-I$NSS_CFLAGS_BASE_PATH/public/nss -I$NSS_CFLAGS_BASE_PATH/$CORE_OBJ_DIR/include" NSS_LIBS="$NSS_CFLAGS_BASE_PATH/$CORE_OBJ_DIR/lib/softokn3.lib advapi32.lib shell32.lib"

   export ZLIB_LIB=$ZLIB_LIB_PATH
   export ZLIB_INCLUDE=$ZLIB_INC_PATH

   ./configure  NSS_CFLAGS="$NSS_CFLAGS" NSS_LIBS="$NSS_LIBS"  --enable-pk11install 

   if [ $? != 0 ];
   then
      echo "Coolkey configure failed...."
      export PATH=${ORIG_PATH}
      return 1
   fi

   make LDFLAGS+="version.lib Setupapi.lib"

   if [ $? != 0 ];
   then
      echo "Can't make coolkey."
      export PATH=${ORIG_PATH}
      return 1

   fi

   cd $BASE_DIR

   # Grab coolkey and zlib

   cp -f coolkey/src/coolkey/.libs/libcoolkeypk11.dll BUILD/coolkeypk11.dll
   cp -f coolkey/src/libckyapplet/.libs/libckyapplet-1.dll BUILD
   cp -f zlib/$ZLIB_DLL.dll BUILD


   # Grab pk11install

   cp -f coolkey/src/install/pk11install.exe BUILD

   export PATH=${ORIG_PATH}
   return 0
}

function obtainZLIB {

  echo "OBTAINING ZLIB..."
  cd $BASE_DIR


  if [ -d ./$ZLIB_NAME ];
  then
      echo "ZLIB already done."
      return 0
  fi

  mkdir -p $ZLIB_NAME

  cd $ZLIB_NAME

  wget $ZLIB_BIN_URL/$ZLIB_ARCHIVE.zip
  if [ $? != 0 ];
  then
    echo "Can't obtain zlib bundle...."
    return 1
  fi

  unzip $ZLIB_ARCHIVE.zip

  if [ $? != 0 ];
  then
    echo "Can't unzip zlib bundle....."
    return 1
  fi

  cp lib/zdll.lib ./$ZLIB_DLL.lib

  return 0
}

function buildESC {

   echo "BUILDING ESC"
   cd $BASE_DIR

   if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doEsc ];
   then
       echo "Do not build ESC."
       return 0
   fi

   if [ -d esc ];
   then
       echo "ESC already checked out.." 
   else 
       cvs  -d $FEDORA_CVS_ROOT co esc 
   fi

   if [ $? != 0 ];
   then
      echo "Can't check out ESC..."
      return 1

   fi

   cd $ESC_NAME

   mkdir -p dist/src

   #mkdir -p dist/$CORE_OBJ_DIR

   cd dist/src


   ZLIB_INC_PATH=${BASE_DIR}/zlib/include
   ZLIB_INC_PATH=`cygpath -m $ZLIB_INC_PATH`

   CKY_INCLUDE_PATH=${BASE_DIR}/coolkey/src/libckyapplet
   CKY_INCLUDE_PATH=`cygpath -m $CKY_INCLUDE_PATH`

   cd ../..
   make BUILD_OPT=1 import

   make BUILD_OPT=1 CKY_INCLUDE="-I$ZLIB_INC_PATH  -I$CKY_INCLUDE_PATH" CKY_LIB_LDD=$CKY_INCLUDE_PATH/.libs USE_XUL_SDK=1 ESC_VERSION=$ESC_VERSION_NO

   if [ $? != 0 ];
   then
      echo "Can't make ESC."
      return 1

   fi

   #take care of eginstall

   cd src/app/eginstall
   make BUILD_OPT=1 install
   cd ../../..

   cp dist/WIN*/coolkey_drivers/egate/eginstall.exe ../BUILD/egate

   if [ $? != 0 ];
   then
       echo "Can't copy egate installer!"
       return 1
    fi
 
   

   # hoist the build into the installer staging area

   cd ..

   cp -rf esc/dist/WIN*/esc_build/ESC BUILD

   if [ $? != 0 ];
   then
      echo "Can't copy ESC to BUILD directory."
      return 1
   fi

   return 0


}

function obtainCAPI {

   cd $BASE_DIR

   echo "OBTAINING the CAPI driver... CSP_PATH $CSP_PATH"


   if [  -z $CSP_PATH ];
   then
       echo "No CSP path specified."
       echo "Set environ var: CSP_PATH if desired . "
       return 0
   fi 

   cp $CSP_PATH/* BUILD

   return 0
}

function obtainEGATE {

   echo "OBTAINING EGINSTALL"

   cd $BASE_DIR


   if [ -d BUILD/egate ];
   then
      echo "Egate already obtained!"
      return 0
   fi
   

   wget $EGATE_DRIVER_URL/$EGATE_DRIVER_NAME
   if [ $? != 0 ];
   then
      echo "Can't obtain egate driver!"
      return 1
   fi
   mkdir -p BUILD/egate
   unzip $EGATE_DRIVER_NAME -d BUILD/egate

   return 0

}

function initializeBUILD {

    cd $BASE_DIR
    echo "INITIALIZING BUILD..."
    mkdir -p BUILD


    if [ $? != 0 ];
    then
       echo "Can't initilize ESC Build...."
       return 1
    fi

    return 0
}

function cleanupBUILD {

    cd $BASE_DIR
    echo "CLEANING up BUILD ..."
    rm -rf *.gz
    #rm -rf *.zip
    return 0

}

function createINSTALLER {
    echo "CREATING INSTALLER..."

    cd $BASE_DIR

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doInstaller ];
    then
        echo "Do not build the installer."
        return 0
    fi

    if [ -z $MSVC_PATH ];
    then
        echo "NO MSVC path specified!"
        echo "Set environ var: MSVC_PATH ."
    fi


    cp $MSVC_PATH/*.dll BUILD

    if [ $? != 0 ];
    then
        echo "Cant copy over MSVC dlls."
    fi

    if [ -z "$INNO_PATH" ];
    then
        echo "No Path to the INNO installer specified!"
        echo "Set environ var: INNO_PATH ."
        return 0
    fi


    #Move over extra files we don't keep in the open source world

    cp esc-image-large.bmp BUILD/ESC/chrome/content/esc

    # Build the INNO executable installer

    "$INNO_PATH" setup.iss

    if [ $? != 0 ];
    then
        echo "Can't build final ESC installer...."
        return 1
    fi

    return 0
}

function usage {

    echo "usage:   ./build.sh [ -doEsc || -doCoolKey || -doNSS || -doInstaller ]"


}

function processARGS {

    if  [ $NUM_ARGS -ne 1 ]  ;
    then
        if [ $NUM_ARGS -ne 0 ];
       then
           echo "Number of args must be 1 or 0 !"
           usage
           exit 1
       fi
    fi


    if [ -z $THE_ARG ];
    then
       return
    fi    
     
    if [ $THE_ARG != -doEsc ] && [ $THE_ARG != -doCoolKey ] && [ $THE_ARG != -doNSS ] && [ $THE_ARG != -doInstaller ];
    then
        echo "Incorrect arguments!"
        usage
        exit 1
    fi

}

########### Main program execution

NUM_ARGS=$#
echo "args   $NUM_ARGS"


THE_ARG=$1



processARGS



initializeBUILD

if [ $? != 0 ];
then
    exit 1
fi

obtainCAPI

if [ $? != 0 ];
then
    exit 1
fi

obtainEGATE

if [ $? != 0 ];
then
    exit 1
fi

buildNSS

if [ $? != 0 ];
then
echo "Issue building NSS."
fi

obtainZLIB

if [ $? != 0 ];
then
    exit 1
fi

buildCOOLKEY

if [ $? != 0 ];
then
    exit 1
fi

buildESC

if [ $? != 0 ];
then
    exit 1
fi

createINSTALLER

if [ $? != 0 ];
then
    exit 1
fi

cleanupBUILD

exit 0
