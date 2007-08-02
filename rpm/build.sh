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


echo "Building ESC...."

RPMBASE=RPMS
RPMPATH=
RPM_OS_TAG=fc8
RPMEXT=i386
RPMDIR=$RPMBASE/$RPMEXT

arch64="x86_64"
thearch=$(uname -m)

#Temporarily installed RPMS

RPM_PCSC_LITE_LIBS=
RPM_PCSC_LITE_DEVEL=
RPM_COOLKEY_DEVEL=
RPM_COOLKEY=
RPM_CCID=
RPM_NSS_DEVEL=
RPM_NSPR_DEVEL=

#URLs for SRPMs if  drawn from the net

FEDORA_SRPMS=http://download.fedora.redhat.com/pub/fedora/linux/development/source/SRPMS/


ESC_SRPM=esc-1.0.1-5.fc8.src.rpm
ESC_URL=$FEDORA_SRPMS/$ESC_SRPM


#####################################################################
################ Cleanup the temporary installed devel packages #####
#####################################################################

function cleanup {

    echo "Cleaning up ..."


}

function getSRPM {

  if [ -z $1 ];
  then
      echo "No input SRPM..."
      return 0;
  else
      echo "Obtaining " $1

      wget -nc  --directory-prefix=SRPMS $1

      if [ $? != 0 ];
      then
          "Echo problem obtaining SRPM $1."
          exit 1
      fi
      
  fi
}

function getRPM {


if [ -z $1 ];
  then
      echo "No input RPM..."
      return 0;
  else
      echo "Obtaining " $1

      wget -nc  --directory-prefix=RPMS/$RPMEXT $1

      if [ $? != 0 ];
      then
          "Echo problem obtaining RPM $1."
          exit 1
      fi

  fi
    
}

function getSRPMS {

    getSRPM $ESC_URL

}


function initializeBUILD {

   mkdir -p BUILD

   if [ $? != 0 ];
   then
       echo "Problem setting up build...."
       exit 1
   fi
  

   mkdir -p RPMS

   if [ $? != 0 ];
   then
       echo "Problem setting up build...."
       exit 1
   fi
 
   mkdir -p SRPMS

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


#####################################################################
################ Main build logic ############################# #####
#####################################################################

initializeBUILD

getSRPMS


#####################################################################
################ Build ESC ##########################################
#####################################################################


echo "Building ESC...."

echo "Obtain latest ESC SRPM..."

echo "Installing ESC SRPM....."

rpm -Uvh --define="_topdir ${PWD}" SRPMS/$ESC_SRPM

if [ $? != 0 ];
then

    echo "Problem installing ESC SRPM...."
    cleanup
    exit 1
fi


rpmbuild -ba --define="_topdir ${PWD}" SPECS/esc.spec

if [ $? != 0 ];
then
    echo "Problem building final ESC RPM..."
  
fi 

cleanup

exit 0
