#!/bin/sh

if [ X$1 == X ]; then
  echo "usage: install.sh <target-dir>"
  exit 1
fi

BASEDIR=`pwd`

echo "Cleaning up base"
chmod 755 base-lint.sh ${1}
./base-lint.sh ${1}

mkdir $1/etc
mkdir $1/etc/init.d
mkdir $1/etc/udev
mkdir $1/etc/vdr
mkdir $1/etc/vdr.d
mkdir $1/etc/vdr/plugins
mkdir $1/etc/vdr/plugins/admin
mkdir $1/etc/vdr/plugins/xineliboutput

cp    --preserve=all -v ./etc/init.d/*                     $1/etc/init.d
cp    --preserve=all -v ./etc/vdr/plugins/admin/*          $1/etc/vdr/plugins/admin
cp    --preserve=all -v ./etc/vdr/plugins/xineliboutput/*  $1/etc/vdr/plugins/xineliboutput
cp    --preserve=all -v ./etc/vdr/*                        $1/etc/vdr
cp    --preserve=all -v ./etc/vdr.d/*                      $1/etc/vdr.d
cp    --preserve=all -v ./etc/*                            $1/etc
cp    --preserve=all -v ./etc/bash/*                       $1/etc/bash
cp    --preserve=all -v ./etc/udev/*                       $1/etc/udev
cp    --preserve=all -r ./root/.xine                       $1/root
cp    --preserve=all -v ./VERSION                          $1/

chmod 755 $1/etc/*.sh
chmod 755 $1/etc/vdr/*.sh
chmod 755 $1/etc/vdr/plugins/admin/*.sh
chmod 755 $1/etc/init.d/*

cp ./usr/bin/svdrpsend $1/usr/bin
chmod 755 $1/usr/bin/svdrpsend

# Record build time.
touch $1/VERSION

cd features
for i in *; do
  [ "${i}" == "CVS" ] && continue
  [ -f ${i} ] && continue
  echo "Cleaning up "$i
  if [ -f ${i}-lint.sh ]; then
    chmod 755 ${i}-lint.sh 
    ./${i}-lint.sh /${1}-${i}
  fi
  echo "Feature: "$i" installing" 
  cd ${i}
  tar cf - . --exclude "CVS" --exclude ".dummy" | tar xvpf - -C /${1}-$i
  cd ..
  chmod 755 /${1}-$i/etc/init.d/S*
  [ -e ${BASEDIR}/ftr-${i}.sqfs ] && rm ${BASEDIR}/ftr-${i}.sqfs
  mksquashfs /${1}-$i ${BASEDIR}/ftr-${i}.sqfs
done
cd ..

[ -e ${BASEDIR}/zentoo-glibc.img ] && rm ${BASEDIR}/zentoo-glibc.img
mksquashfs /${1} ${BASEDIR}/zentoo-glibc.img
