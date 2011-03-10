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

### USE_64     -  Are we trying to build the 64 bit version 

NUM_ARGS=0
ARG_COMMAND=


# NSS values
# NSS needed just to help coolkey build

NSS_NAME=nss-3.12.5
NSPR_NAME=nspr-4.8.2
NSS_PATH_NAME=NSS_3_12_5_RTM
NSS_ARCHIVE=$NSS_NAME-with-$NSPR_NAME
NSS_SOURCE_URL=https://ftp.mozilla.org/pub/mozilla.org/security/nss/releases/$NSS_PATH_NAME/src/$NSS_ARCHIVE.tar.gz

NSS_NAME=$NSS_ARCHIVE
NSS_LIB_PATH=$NSS_NAME/mozilla/dist/WIN*/lib

#Inno installer values

#INNO_PATH="C:/Program Files/Inno Setup 5/ISCC.exe"


#Zlib values

ZLIB_NAME=zlib
ZLIB_DLL=zlib1


ZLIB_DLL_64=zlibwapi
ZLIB_ARCHIVE=zlib123-dll
ZLIB_BIN_URL=http://sourceforge.net/projects/libpng/files/zlib/1.2.3

ZLIB_ARCHIVE_64=zlib123dllx64
ZLIB_BIN_URL_64=http://winimage.com/zLibDll

#CoolKey values

COOLKEY_NAME=coolkey
COOLKEY_TAG=HEAD

PKI_PATH=http://pki.fedoraproject.org/pki
CSP_PATH=support/esc/windows/csp/32/latest
CSP_DIR=CLKCSP
CSP_ARCHIVE=CLKCSP.zip

if [ X$ON_64 == X1 ];
then
export INNO_PATH="C:/Program Files (x86)/Inno Setup 5/ISCC.exe"
fi


if [ X$USE_64 == X1 ];
then
CSP_PATH=support/esc/windows/csp/64/latest
fi


#Fedora repo for CoolKey and ESC

FEDORA_CVS_ROOT=:pserver:anonymous@cvs.fedoraproject.org:/cvs/dirsec

#Xulrunner values


#Xulrunner SDK
MOZILLA_FTP_PATH=ftp://ftp.mozilla.org/pub/mozilla.org
XULRUNNER_SDK_PATH=xulrunner/releases/1.9.0.17/sdk/
XULRUNNER_SDK_ARCHIVE=xulrunner-1.9.0.17.en-US.win32.sdk.zip

XUL_SDK_DIR=xulrunner-sdk

#Xlrunner runtime

XULRUNNER_DIR=xulrunner
XULRUNNER_FTP_PATH=http://releases.mozilla.org/pub/mozilla.org/
XULRUNNER_PATH=xulrunner/releases/1.9.2.15/runtimes/

XULRUNNER_ARCHIVE=xulrunner-1.9.2.15.en-US.win32.zip


#Base Dirctory calc

BASE_DIR=${PWD}


#ESC values

ESC_NAME=esc
ESC_VERSION_NO=1.1.0-12


#Cygwin values

ORIG_PATH=${PATH}

CYGWIN_BIN_PATH=/cygdrive/c/cygwin/bin

#Value for the location of Moz Tools needed to compile

MOZ_TOOLS_BIN_PATH=/cygdrive/c/moztools/bin:/cygdrive/d/moztools/bin

export PATH=${MOZ_TOOLS_BIN_PATH}:${ORIG_PATH}

#CORE_OBJ_DIR=`uname``uname -r`_OPT.OBJ
CORE_OBJ_DIR=WINNT5.2_OPT.OBJ

export PATH=${ORIG_PATH}

GECKO_SDK_PATH=${BASE_DIR}/${XUL_SDK_DIR}


function buildNSS  {

    echo "BUILDING NSS..."

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doNSS ] || [ X$USE_64 == X1 ];
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
    fi

    if [ $? != 0 ];
    then
        echo "Can't untar NSS."
        return 1
    fi

    cd $NSS_NAME/mozilla/security/nss
    make BUILD_OPT=1 nss_build_all
    if [ $? != 0 ];
    then
        echo "Can't make nss."
        return 1
        
    fi

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


   if [ X$USE_64 == X1 ];
   then
     ZLIB_LIB_PATH=${BASE_DIR}/zlib/dll_x64
     ZLIB_LIB_FLAGS=${BASE_DIR}/zlib/dll_x64/$ZLIB_DLL_64.dll
   else
     ZLIB_LIB_FLAGS=${BASE_DIR}/zlib/$ZLIB_DLL.dll
   fi

   ZLIB_INC_PATH=`cygpath -m $ZLIB_INC_PATH`
   ZLIB_LIB_PATH=`cygpath -m $ZLIB_LIB_PATH`

   ZLIB_LIB_FLAGS=`cygpath -m $ZLIB_LIB_FLAGS` 

   NSS_CFLAGS_BASE_PATH=${BASE_DIR}/$NSS_NAME/mozilla/dist

   NSS_CFLAGS_BASE_PATH=`cygpath -m $NSS_CFLAGS_BASE_PATH`


   NSS_CFLAGS="-I$NSS_CFLAGS_BASE_PATH/public/nss -I$NSS_CFLAGS_BASE_PATH/$CORE_OBJ_DIR/include" NSS_LIBS="$NSS_CFLAGS_BASE_PATH/$CORE_OBJ_DIR/lib/softokn3.lib advapi32.lib shell32.lib"

   export ZLIB_LIB=$ZLIB_LIB_PATH
   export ZLIB_INCLUDE=$ZLIB_INC_PATH

   if [ X$USE_64 == X1 ];
   then
       PK11=
   else
       PK11=--enable-pk11install
   fi

   ./configure  NSS_CFLAGS="$NSS_CFLAGS" NSS_LIBS="$NSS_LIBS"  $PK11 

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

   if [ X$USE_64 == X1 ];
   then
      cp -f zlib/dll_x64/zlibwapi.dll BUILD
   else
      cp -f zlib/$ZLIB_DLL.dll BUILD
   fi


   # Grab pk11install

   if [ X$USE_64 != X1 ];
   then
      cp -f coolkey/src/install/pk11install.exe BUILD
   fi

   export PATH=${ORIG_PATH}
   return 0
}

function obtainXULSDK {

  if [ X$USE_64 == X1 ];
  then
      echo "Don't get XUL SDK for 64 bits."
      return 0
  fi

  if [ -d ${XUL_SDK_DIR} ];
  then
      echo "XUL SDK already obtained."
      return 0
  fi


  wget ${MOZILLA_FTP_PATH}/${XULRUNNER_SDK_PATH}/${XULRUNNER_SDK_ARCHIVE}

  if [ $? != 0 ];
  then
     echo "Can't download the XUL SDK."
     return 1
  fi

  unzip ${XULRUNNER_SDK_ARCHIVE}

  if [ $? != 0 ];
  then
     echo "Can't unzip XUL SDK."
     return 1
  fi

  chmod -R 755  ${XUL_SDK_DIR}/bin/*.exe
  chmod -R 755  ${XUL_SDK_DIR}/bin/*.dll


  rm -f ${XULRUNNER_SDK_ARCHIVE}


  if [ -d ${XULRUNNER_DIR} ];
  then
      echo "XULRUNNER already obtained."
  fi

  wget ${XULRUNNER_FTP_PATH}/${XULRUNNER_PATH}/${XULRUNNER_ARCHIVE}

  if [ $? != 0 ];
  then
      echo "Can't download Xulrunner Runtime."
      return 1
  fi

  unzip ${XULRUNNER_ARCHIVE}

  if [ $? != 0 ];
  then
      echo "Can't unzip Xulrunner Runtime."
      return 1
  fi

  rm -f ${XULRUNNER_ARCHIVE}

  chmod -R 755  ${XULRUNNER_DIR}/*.exe
  chmod -R 755  ${XULRUNNER_DIR}/*.dll 

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

  if [  X$USE_64 == X1 ]
  then
   wget $ZLIB_BIN_URL_64/$ZLIB_ARCHIVE_64.zip

   if [ $? != 0 ];
   then
     echo "Can't obtain zlib 64 bit bundle...."
     return 1
   fi

   unzip $ZLIB_ARCHIVE_64.zip

   if [ $? != 0 ];
   then
     echo "Can't obtain zlib 64 bit bundle...."
   fi 

   rm -f README.txt

   cp dll_x64/zlibwapi.lib dll_x64/zdll.lib 

  fi

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

   if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doEsc ] || [ X$USE_64 == X1 ];
   then
       echo "Do not build ESC."
       return 0
   fi

   export GECKO_SDK_PATH=`cygpath -m $GECKO_SDK_PATH`

   echo "GECKO_SDK_PATH ${GECKO_SDK_PATH}"

   cd ../ 

   mkdir -p dist/src

   #mkdir -p dist/$CORE_OBJ_DIR

   cd dist/src


   ZLIB_INC_PATH=${BASE_DIR}/zlib/include
   ZLIB_INC_PATH=`cygpath -m $ZLIB_INC_PATH`

   CKY_INCLUDE_PATH=${BASE_DIR}/coolkey/src/libckyapplet
   CKY_INCLUDE_PATH=`cygpath -m $CKY_INCLUDE_PATH`

   cd ../..
   make OS_RELEASE=5.1 BUILD_OPT=1 import

   make OS_RELEASE=5.1 BUILD_OPT=1 CKY_INCLUDE="-I$ZLIB_INC_PATH  -I$CKY_INCLUDE_PATH" CKY_LIB_LDD=$CKY_INCLUDE_PATH/.libs USE_XUL_SDK=1 ESC_VERSION=$ESC_VERSION_NO

   if [ $? != 0 ];
   then
      echo "Can't make ESC."
      return 1

   fi

   #take care of eginstall

   cd src/app/eginstall
   make BUILD_OPT=1 install
   cd ../../..


   # hoist the build into the installer staging area


   cp -rf dist/WIN*/esc_build/ESC win32/BUILD

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

   if [ -d ${CSP_DIR} ];
   then
      echo "CSP already obtained."
      return 0
   fi

   wget ${PKI_PATH}/${CSP_PATH}/${CSP_ARCHIVE}

   if [ $? != 0 ];
   then
     echo "Can't download the CSP."
     return 1
   fi

   unzip ${CSP_ARCHIVE}

   rm -f ${CSP_ARCHIVE}

   if [ $? != 0 ];
   then
      echo "Can't unzip the CSP."
      return 1
   fi

   cp $CSP_DIR/* BUILD

   if [ $? != 0 ];
   then
     echo "Unable to obtain CSP driver!"
     return 1
   fi

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
        return 1
    fi


    cp $MSVC_PATH/* BUILD

    if [ $? != 0 ];
    then
        echo "Cant copy over MSVC dlls."
    fi

    if [ -z "$INNO_PATH" ];
    then
        echo "No Path to the INNO installer specified!"
        echo "Set environ var: INNO_PATH ."
        return 1 
    fi


    #Move over extra files

    if [ X$USE_64 == X1 ];
    then
       INNO_SCRIPT=coolkey-64.iss
    else
        cp esc-image-large.bmp BUILD/ESC/chrome/content/esc


        #Transport the nss files needed for pk11install.exe

        cp $NSS_LIB_PATH/softokn3.dll BUILD
        cp $NSS_LIB_PATH/libplc4.dll BUILD
        cp $NSS_LIB_PATH/libnspr4.dll BUILD
        cp $NSS_LIB_PATH/libplds4.dll BUILD
        cp $NSS_LIB_PATH/nssutil3.dll BUILD
        cp $NSS_LIB_PATH/sqlite3.dll BUILD

        INNO_SCRIPT=setup.iss
    fi


    # Build the INNO executable installer

    "$INNO_PATH" $INNO_SCRIPT 

    if [ $? != 0 ];
    then
        echo "Can't build final ESC/Coolkey installer...."
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

obtainXULSDK

if [ $? != 0 ];
then 
    exit 1
fi


obtainCAPI

if [ $? != 0 ];
then
    exit 1
fi

if [ $? != 0 ];
then
    exit 1
fi

buildNSS

if [ $? != 0 ];
then
    echo "Issue building NSS."
    exit 1
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
