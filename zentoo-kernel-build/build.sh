#!/bin/bash

BASEDIR=`pwd`

if [ -e ./prebuild/modules-2.6.36.tgz ]; then
  echo You have pre-built modules and kernel.
  echo if you want to rebuild them, delete them first
  exit 0
fi

rm /usr/src/linux 
ln -sf $BASEDIR/linux-2.6.36-aufs /usr/src/linux

cd linux-2.6.36-aufs
make bzImage
make modules
make modules_install

cd $BASEDIR

cp linux-2.6.36-aufs/arch/x86/boot/bzImage prebuild/ 
(cd /lib/modules;tar cvzf $BASEDIR/prebuild/modules-2.6.36.tgz 2.6.36)

