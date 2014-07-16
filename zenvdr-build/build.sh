#!/bin/bash

# VDR build script for zen-projects
# the zen-projects now use a new build system to faciliate
# generating updates from new VDR plugins and packages.
# This build system uses a single file for one or many 
# additions to the build tree.
# Each file contains three scriptlets to prepare, install and
# provide confguration hints to the build script.
#
# 

# Some default settings

TEMP_DIR=/tmp
DL_DIR=`pwd`/dl-files
SRC_DIR=`pwd`/src-files
PLG_DIR=`pwd`/plugin-tar
TESTING_DIR=`pwd`/build.d/testing


[ ! -e ${DL_DIR} ] && mkdir ${DL_DIR}
[ ! -e ${PLG_DIR} ] && mkdir ${PLG_DIR}

# Resource files - They decide what to build

if [ X$RESOURCE == X ]; then
  RESOURCE=default
fi

echo "Populating build.d with $RESOURCE files."
cd build.d
. $RESOURCE.resource
if [ -e ./testing/testing.resource ]; then
  . ./testing/testing.resource
fi
cd -

for i in ./build.d/???-* 
do
  echo "Will build: ${i}"
done

#read -p "Press enter to continue"

# Prepare all selected packages for installation by calling the 
# 'prep' function of each package configuration and install script
# when all scripts have been run successfully we can go ahead and
# compile VDR

for i in ./build.d/???-* 
do
  . ${i} prep
done

# Make sure that a VDR package was prepared on the system

if [ X${VDR_DIR} == X ]; then
  echo "VDR directory variable not set - cannot continue !"
  exit 2
fi

# Compile VDR and all plugins.

cd ${VDR_DIR}

make
RC=$?
if [ $RC != 0 ]; then
  echo "Error ( see above )"
fi 

make plugins
RC=$?
if [ $RC != 0 ]; then
  echo "Error ( see above )"
fi 

cd ..

# Call each package's install script to make sure everything is installed
# properly
#
# The install process walks the files from back to front, so that plugins
# may remove files before another component installs them by accident.

rm /tmp/tmplist
for i in ./build.d/???-*; do
  echo  ${i} >> /tmp/tmplist
done
LIST=`cat /tmp/tmplist | sort -r`


for i in ${LIST}
do
  if [ X$METHOD == Xtar ]; then
    if echo ${i} | grep -q plg ; then
      OLD_TARGET=${TARGET_DIR}
      mkdir /tmp/plugin
      TARGET_DIR=/tmp/plugin
      . ${i} install
      . ${i} config
      cd ${TARGET_DIR}
      
      tar cvzf ${PLG_DIR}/plg-${PACKAGE_NAME}-${PACKAGE_VER}.tgz .
      
      cd -
      rm -rf /tmp/plugin
      TARGET_DIR=${OLD_TARGET}
    else
      . ${i} install
      . ${i} config
    fi
  else
    . ${i} install
    . ${i} config
  fi
done

