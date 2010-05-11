#!/bin/bash

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
 # END COPYRIGHT BLOCK **/


# Build Mac ESC/CoolKey/IFD-EGATE/LIB-USB bundle
# Error reported by PackageMaker such as:
# : for architecture i386 object: updater malformed object (unknown flavor for flavor number 0 in LC_UNIXTHREAD command 14 can't byte swap it)

# Has been reported to only happen on a PPC system and is benign...


#Environment variables
# TOKEND_PATH_NAME  - Path of where to obtain the TokenD bundle ex: /usr/local/tokend/COOLKEY.zip

#GECKO_SDK_PATH - Path to the Universal Binary Xulrunner SDK

if [ ! $XUL_FRAMEWORK_PATH ];
then
    XUL_FRAMEWORK_PATH=~/XUL.framework
fi

TOKEND_PATH_NAME=/Users/slowjack/COOLKEY.zip

printf "\n \n"
echo "Building ESC... for Mac.... "
printf "\n"

COOLKEY_PKG_NAME=SmartCardManager1.19.pkg
COOLKEY_VOL_NAME=SMARTCARDMANAGER
COOLKEY_TAG=HEAD
ESC_TAG=HEAD

ESC_VERSION=1.1.0-12

COOLKEY_DMG_NAME=SmartCardManager-$ESC_VERSION.OSX5.darwin.dmg

ENABLE_PK11INSTALL=--enable-pk11install

#replacement libtool files

LIBTOOL_COOLKEY=

#Various CVS repositories

FEDORA_CVS_ROOT=:pserver:anonymous@cvs.fedora.redhat.com/cvs/dirsec
MOZ_CVS_ROOT=:pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot

# Various path constants

PACKAGE_MAKER_PATH=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS
PK11INSTALL_PATH=/Applications/Utilities/PK11Install

TOKEND_DEST_PATH=/System/Library/Security/tokend
TOKEND_DEST_NAME=COOLKEY.tokend

BASE_DIR=${PWD}

COOLKEY_PATH=/usr/local/CoolKey

#Where to grab a few universal NSS dylib's for pk11install

PK11INSTALL_LIB_PATH=$GECKO_SDK_PATH/bin


function cleanup {

    cd $BASE_DIR

    echo "Cleaning up.... "
    printf "\n"

    rm -f *.gz

    rm -rf $COOLKEY_PKG_NAME

    rm  -f COOLKEY.zip

    sudo rm -rf COOLKEY.tokend

}

function buildCOOLKEY {

    echo "Build CoolKey... "
    printf "\n"

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doCoolKey ];
    then
       echo "Do not build CoolKey"
       return 0
    fi

    echo "ENABLE_PK11INSTALLL $ENABLE_PK11INSTALL"

    cd $BASE_DIR

    if [ -d coolkey ];
    then
       echo "CoolKey checked out already."
    else
       cvs   -d $FEDORA_CVS_ROOT co -r $COOLKEY_TAG coolkey
    fi
  
    if [ $? != 0 ];
    then
        echo "Can't checkout CoolKey code..."
        return 1
    fi

    cd coolkey

    /usr/bin/autoconf

    if [ $? != 0 ];
    then
        echo "CoolKey autoconf failed..."
        return 1
    fi

    ./configure --enable-debug  --disable-dependency-tracking --prefix=$COOLKEY_PATH NSS_CFLAGS="-I  $GECKO_SDK_PATH/sdk/include" NSS_LIBS="-L/Library/Frameworks/XUL.framework/Versions/Current -Wl,-executable_path,/System/Frameworks//XUL.framework/Versions/Current $ENABLE_PK11INSTALL"

    if [ $? != 0 ];
    then
        echo "Coolkey configure failed..."
        return 1
    fi

    cp ../misc/libtool.coolkey.patch . 
    patch -p0 -N < libtool.coolkey.patch

    make clean
    make

    if [ $? != 0 ];
    then
        echo "Can't make coolkey."
        return 1
    fi

    if [ $ENABLE_PK11INSTALL ];
    then
        echo "Making PK11INSTALL!!!! ......"
        cd src/install
        make

        if [ $? != 0 ];
        then
            echo "Can't make pk11install!"
           return 1
        fi
 
        make install DESTDIR=$BASE_DIR/staging
        cd ../..
    fi


    make install DESTDIR=$BASE_DIR/staging

   # Manually give pk11install the NSS/NSPR dylibs it needs

   cp -f $PK11INSTALL_LIB_PATH/libsoftokn3.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libplc4.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libplds4.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libnspr4.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libnssutil3.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libsqlite3.dylib ../staging/$COOLKEY_PATH/bin
   cp -f $PK11INSTALL_LIB_PATH/libnssdbm3.dylib ../staging/$COOLKEY_PATH/bin

   return 0
}


function buildESC {

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doEsc ];
    then
       echo "Do not build ESC"
       return 0
    fi


    echo "Build ESC ... " 

    printf "\n"

   cd $BASE_DIR

   if [ -d esc ];
   then
      echo "ESC checked out already."
   else
      cvs -d $FEDORA_CVS_ROOT co -r $ESC_TAG esc
   fi

   if [ $? != 0 ];
    then
        echo "Can't checkout  ESC code."
        return 1
    fi

   cd esc

   make BUILD_OPT=1 USE_XUL_SDK=1 clean
   echo   make BUILD_OPT=1 USE_XUL_SDK=1 ESC_VERSION=$ESC_VERSION CKY_INCLUDE=-I$BASE_DIR/staging/$COOLKEY_PATH/include CKY_LIB_LDD=-L$BASE_DIR/staging/$COOLKEY_PATH/lib XUL_FRAMEWORK_PATH=$XUL_FRAMEWORK_PATH

   echo   make BUILD_OPT=1 USE_XUL_SDK=1 ESC_VERSION=$ESC_VERSION CKY_INCLUDE=-I$BASE_DIR/staging/$COOLKEY_PATH/include CKY_LIB_LDD=-L$BASE_DIR/staging/$COOLKEY_PATH/lib XUL_FRAMEWORK_PATH=$XUL_FRAMEWORK_PATH > build.sh

   echo make BUILD_OPT=1 USE_XUL_SDK=1 clean > clean.sh

   chmod 775 build.sh
   chmod 775 clean.sh

   make BUILD_OPT=1 USE_XUL_SDK=1 ESC_VERSION=$ESC_VERSION CKY_INCLUDE=-I$BASE_DIR/staging/$COOLKEY_PATH/include CKY_LIB_LDD=-L$BASE_DIR/staging/$COOLKEY_PATH/lib XUL_FRAMEWORK_PATH=$XUL_FRAMEWORK_PATH

   if [ $? != 0 ];
    then
        echo "Can't make ESC."
        return 1
    fi

   rsync -rl dist/Dar*/esc_build/ESC.app ../staging/Applications

   return 0
}

function obtainTokenD {

echo "TOKEND_PATH_NAME $TOKEND_PATH_NAME"

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doTokenD ];
    then
       echo "Do not obtain TokenD"
       return 0
    fi


    cd $BASE_DIR

    echo "Obtaining the Mac TokenD driver...."

    printf "\n"


   if [ ! -f $TOKEND_PATH_NAME ];
   then

       echo "TokenD not found!..."

       return 0;

   fi
  
   echo "TokenD found.... "
   printf "\n"

   cp $TOKEND_PATH_NAME .


   if [ $? != 0 ];
    then
        echo "Can't find the TokenD zip file!"
        return 0
    fi


   mkdir -p staging/$TOKEND_DEST_PATH

   unzip COOLKEY.zip

   if [ $? != 0 ];
    then
        echo "Can't unzip TokenD!"
        return 0
    fi

   sudo mv COOLKEY.tokend ./staging/$TOKEND_DEST_PATH/COOLKEY.tokend

   if [ $? != 0 ];
    then
        echo "Can't rename TokenD bundle!."
        return 0
    fi

    return 0
}

function buildMacPackage {

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doInstaller ];
    then
       echo "Do not build SmartCardManager installer"
       return 0
    fi

    cd $BASE_DIR
    echo "Building final Mac installer package... "
    printf "\n"


    mkdir -p staging/$PK11INSTALL_PATH
    cp coolkey_package_data/pk11install.command staging/$PK11INSTALL_PATH

    cp $BASE_DIR/coolkey_package_data/es-client.icns $BASE_DIR/staging/Applications/ESC.app/Contents/Resources/xulrunner.icns
    
    defaults write $BASE_DIR/staging/Applications/ESC.app/Contents/Info "CFBundleIconFile" 'xulrunner.icns'

    if [ $? != 0 ];
    then
        echo "Can't configure ESC for custom icon file."
        return 1
    fi


    # Get rid of stray .DS_Store files

    find ./staging -name ".DS_Store" -exec rm -f {} \;


    # Set bunch of permissions

     echo "Setting file permissions.." 
     sudo chown -R -v -h root:wheel staging/usr
     sudo chown -R -v -h root:wheel staging/System
     sudo chown -R -v -h root:admin staging/Applications

     sudo chmod -R 755  staging/usr
     sudo chmod -R 775  staging/Applications
     sudo chmod -R 755  staging/System

     echo "About to create pkg installer..."

    $PACKAGE_MAKER_PATH/PackageMaker -build -p $COOLKEY_PKG_NAME -f $BASE_DIR/staging -i $BASE_DIR/coolkey_package_data/Info.plist -d $BASE_DIR/coolkey_package_data/Description.plist -r $BASE_DIR/coolkey_package_data/Resources --verbose

    if [ $? != 0 ];
    then
        echo "Can't build CoolKey installer package."
        return 1
    fi

    echo "Creating final dmg file .... "
    printf "\n"

    hdiutil create -format UDZO -fs HFS+ -volname $COOLKEY_VOL_NAME -srcfolder $BASE_DIR/$COOLKEY_PKG_NAME $COOLKEY_DMG_NAME

    if [ $? != 0 ];
    then
        echo "Can't create final ESC DMG archive."
        return 1
    fi

    return 0

}

function initializeBuild {

   echo "Initializing system for Mac build..... "
   printf "\n"

   mkdir -p staging
   mkdir -p staging/usr
   mkdir -p staging/System
   mkdir -p staging/Applications

   sudo chown -R -v -h ${USER}:staff staging/usr
   sudo chown -R -v -h ${USER}:staff staging/System
   sudo chown -R -v -h ${USER}:staff staging/Applications

   rm -rf staging/CVS

   rm -rf  staging/$TOKEND_DEST_PATH

   rm -rf  staging/Applications/*
   rm -rf  staging/System/*
   rm -rf  stating/usr/*

   rm -rf *.dmg

   export MACOSX_DEPLOYMENT_TARGET=10.5

}

function usage {

    echo "usage:   ./build.sh [ -doUsb || -doEgate   -doEsc || -doCoolKey || -doTokenD ||  -doInstaller ]"

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

    if [ $THE_ARG != -doEsc ] && [ $THE_ARG != -doCoolKey ] && [ $THE_ARG != -doTokenD ] &&  [ $THE_ARG != -doInstaller ];
    then
        echo "Incorrect arguments!"
        usage
        exit 1
    fi
}


##################################### main ##############################################################

NUM_ARGS=$#

THE_ARG=$1

processARGS

initializeBuild

if [ $? != 0 ];
then
    exit 1 
fi

if [ $? != 0 ];
then
    exit 1
fi

# Build coolkey because ESC needs it

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

obtainTokenD

if [ $? != 0 ];
then
    exit 1
fi

buildMacPackage

if [ $? != 0 ];
then
    exit 1
fi

cleanup
