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

printf "\n \n"
echo "Building ESC... for Mac.... "
printf "\n"

OSX_RPM_PATH=/usr/local/bin

LIB_USB_URL_BASE=http://downloads.sourceforge.net/libusb
LIB_USB_NAME=libusb-0.1.12
LIB_USB_URL=$LIB_USB_URL_BASE/$LIB_USB_NAME

IFD_EGATE_URL_BASE=ftp://download.fedora.redhat.com/pub/fedora/linux/core/6/source/SRPMS
IFD_EGATE_NAME=ifd-egate-0.05
IFD_EGATE_REL=15

COOLKEY_PKG_NAME=SmartCardManager1.16.pkg
COOLKEY_VOL_NAME=SMARTCARDMANAGER
COOLKEY_TAG=HEAD


ESC_VERSION=1.0.1-6

COOLKEY_DMG_NAME=SmartCardManager-$ESC_VERSION.OSX4.darwin.dmg

ENABLE_PK11INSTALL=

#replacement libtool files

LIBTOOL_USB_PATCH=../misc/libtool.usb.patch
LIBTOOL_COOLKEY=

#Various CVS repositories

FEDORA_CVS_ROOT=:pserver:anonymous@cvs.fedora.redhat.com/cvs/dirsec
MOZ_CVS_ROOT=:pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot
MOZ_XULRUNNER_BRANCH=MOZILLA_1_8_0_7_RELEASE

# Various path constants

PACKAGE_MAKER_PATH=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS
PK11INSTALL_PATH=/Applications/Utilities/PK11Install

TOKEND_DEST_PATH=/System/Library/Security/tokend
#TOKEND_PATH_NAME=/share/builds/components/tokend/20070111/COOLKEY.zip
TOKEND_DEST_NAME=A_COOLKEY.tokend

BASE_DIR=${PWD}

COOLKEY_PATH=/usr/local/CoolKey

#Where to grab a few universal NSS dylib's for pk11install

PK11INSTALL_LIB_PATH=$BASE_DIR/esc/dist/Darwin6.8_OPT.OBJ/xulrunner_build/i386/dist/universal/xulrunner/XUL.framework/Versions/Current


function cleanup {

    cd $BASE_DIR

    echo "Cleaning up.... "
    printf "\n"

    rm -f *.gz

    rm  -f COOLKEY.zip

}

function buildUSB {

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doUsb ];
    then
       echo "Do not build Usb"
       return 0
    fi

    cd $BASE_DIR

    echo "Build Lib USB... "
    printf "\n"

    curl --verbose -O -L $LIB_USB_URL_BASE/$LIB_USB_NAME.tar.gz

    if [ $? != 0 ];
    then
        echo "Can't obtain tarball for Lib USB."
        return 1
    fi


    tar -xzvf $LIB_USB_NAME.tar.gz 

    if [ $? != 0 ];
    then
        echo "Can't unpack Lib USB tarball."
        return 1
    fi

    cd $LIB_USB_NAME

    ./configure --disable-dependency-tracking  --prefix=$COOLKEY_PATH CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc" CXXFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc"  LDFLAGS="-arch ppc -arch i386"

    if [ $? != 0 ];
    then
        echo "Can't configure Lib USB."
        return 1
    fi

    cp $LIBTOOL_USB_PATCH . 
    patch -p0 -N < libtool.usb.patch 

    make

    if [ $? != 0 ];
    then
        echo "Can't make Lib USB."
        return 1
    fi


    make DESTDIR=${PWD}/../staging install

    return 0
}

function buildEGATE {

    if [ $NUM_ARGS -ne 0 ] && [ $THE_ARG != -doEgate ];
    then
       echo "Do not build Egate"
       return 0
    fi

    echo "Build IFD-EGATE ... "

    printf "\n"

    cd $BASE_DIR

    curl --verbose -O  $IFD_EGATE_URL_BASE/$IFD_EGATE_NAME-$IFD_EGATE_REL.src.rpm

    if [ $? != 0 ];
    then
        echo "Can't obtain RPM for Egate."
        return 1
    fi


    $OSX_RPM_PATH/rpm -ihv --define="_topdir ${PWD}" $IFD_EGATE_NAME-$IFD_EGATE_REL.src.rpm

    $OSX_RPM_PATH/rpmbuild --nodeps -bp --define="_topdir ${PWD}" SPECS/ifd-egate.spec 

    cd BUILD/$IFD_EGATE_NAME

    make  PCSC_CFLAGS=-I/System/Library/Frameworks/PCSC.framework/Versions/Current/Headers USB_CFLAGS="-I../../staging/usr/local/CoolKey/include -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch ppc -arch i386" USB_LDFLAGS="-L../../staging/usr/local/CoolKey/lib -arch ppc -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386"  -f Makefile-OSX 

    if [ $? != 0 ];
    then
        echo "Can't buld Egate."
        return 1
    fi

    cp ../../misc/Makefile-OSX.egate.patch .
    patch -p0 -N < Makefile-OSX.egate.patch 

    make -f Makefile-OSX DESTDIR=${PWD}/../../staging install

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

    cvs   -d $FEDORA_CVS_ROOT co -r $COOLKEY_TAG coolkey
  
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

    ./configure  --disable-dependency-tracking --prefix=$COOLKEY_PATH NSS_CFLAGS="-I ${PWD}/../esc/dist/Dar*/xulrunner_build/i386/dist/public/nss -I ${PWD}/../esc/dist/Dar*/xulrunner_build/i386/dist/include/nspr" NSS_LIBS="-L${PWD}/../esc/dist/Darwin6.8_OPT.OBJ/xulrunner_build/i386/dist/universal/xulrunner/XUL.framework/Versions/Current -Wl,-executable_path,${PWD}/../esc/dist/Darwin6.8_OPT.OBJ/xulrunner_build/i386/dist/universal/xulrunner/XUL.framework/Versions/Current"

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
            echo "Can't re-make coolkey."
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
      cvs  -d  $FEDORA_CVS_ROOT update esc
   else
      cvs -d $FEDORA_CVS_ROOT co esc
   fi

   if [ $? != 0 ];
    then
        echo "Can't checkout  ESC code."
        return 1
    fi

   cd esc
   mkdir -p dist/src
   cd dist/src

   cvs -d $MOZ_CVS_ROOT co -r  $MOZ_XULRUNNER_BRANCH mozilla/client.mk

   if [ $? != 0 ];
    then
        echo "Can't checkout Xulrunner code."
        return 1
    fi

   cd mozilla
   make -f client.mk checkout MOZ_CO_PROJECT=xulrunner
   
   if [ $? != 0 ];
    then
        echo "Can't checkout Xulrunner code."
        return 1
    fi
 
   cd ../../..

   make BUILD_OPT=1 ESC_VERSION=$ESC_VERSION CKY_INCLUDE=-I$BASE_DIR/staging/$COOLKEY_PATH/include CKY_LIB_LDD=-L$BASE_DIR/staging/$COOLKEY_PATH/lib

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


   mv COOLKEY.tokend ./staging/$TOKEND_DEST_PATH/A_COOLKEY.tokend

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

    $PACKAGE_MAKER_PATH/PackageMaker -build -p $COOLKEY_PKG_NAME -f $BASE_DIR/staging -i $BASE_DIR/coolkey_package_data/Info.plist -d $BASE_DIR/coolkey_package_data/Description.plist -r $BASE_DIR/coolkey_package_data/Resources

    if [ $? != 0 ];
    then
        echo "Can't build CoolKey installer package."
        return 1
    fi

    echo "Creating final dmg file .... "
    printf "\n"

    hdiutil create -format UDZO -fs HFS+ -volname $COOLKEY_VOL_NAME -srcfolder $COOLKEY_PKG_NAME $COOLKEY_DMG_NAME

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


   sudo chown -R -v -h ${USER}:${USER} staging/usr
   sudo chown -R -v -h ${USER}:${USER} staging/System
   sudo chown -R -v -h ${USER}:${USER} staging/Applications


   echo "Setting default compiler to gcc 4.0.1 ...... "
   printf "\n"

   sudo gcc_select 4.0

   rm -rf staging/CVS

   rm -rf  staging/$TOKEND_DEST_PATH

   rm -rf *.dmg


   mkdir -p BUILD

   if [ $? != 0 ];
   then
       echo "Problem setting up build...."
       exit 1
   fi

   mkdir -p SPECS

   if [ $? != 0 ];
   then
       echo "Problem setting up build...."
       exit 1
   fi
   mkdir -p SOURCES

   if [ $? != 0 ];
   then
       echo "Problem setting up build...."
       exit 1
   fi


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

    if [ $THE_ARG != -doUsb ] && [ $THE_ARG != -doEgate ] && [ $THE_ARG != -doEsc ] && [ $THE_ARG != -doCoolKey ] && [ $THE_ARG != -doTokenD ] &&  [ $THE_ARG != -doInstaller ];
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

buildUSB

if [ $? != 0 ];
then
    exit 1 
fi

buildEGATE

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

# Build coolkey, now with pk11install

ENABLE_PK11INSTALL=--enable-pk11install

buildCOOLKEY

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
