#!/bin/bash

echo Making Distro
echo
echo Compressing rom image
cat ./zenvdr-init/zentoo-glibc.img | bzip2 > ./distro/zen2vdr.rom.bz2
cp ./zenvdr-init/VERSION ./distro

echo Copying kernel
cp ./zentoo-kernel-build/prebuild/bzImage ./distro

echo Copying modules
cp ./zentoo-kernel-build/prebuild/modules-2.6.36.tgz ./distro

cd distro

echo "make sure DOM is present as /dev/sda"
echo PRESS ENTER TO CONTINE CTRL+C TO STOP
read foo

echo Installing kernel and lilo
mount /dev/sda1 mnt
cp bzImage mnt/boot/bzImage 
cp modules-2.6.36.tgz mnt/boot/modules.tgz 
lilo -r mnt -b /dev/sda 

echo Cleaning updates and conf
rm  -f  mnt/updates/*
rm  -rf mnt/conf/*
cd mnt 
tar czvf ../hda1.tgz . --exclude lost+found
cd ..
umount mnt

echo Installing ROM image
bzcat zen2vdr.rom.bz2 > /dev/sda2

echo Saving DOM image
dd if=/dev/sda of=zen2vdr-r2.img bs=512
