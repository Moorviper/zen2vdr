#!/bin/bash

echo Lint: $1

echo "Cleaning portage db dirs."
rm -rf ${1}/var/db
rm -rf ${1}/var/cache/edb

########################################################################
# löscht alle files, die für Englisch/Deutsch nicht benötigt werden.
########################################################################

#  -------zoneinfo-----------------
mkdir ${1}/usr/share/tmp
mv ${1}/usr/share/zoneinfo/Europe ${1}/usr/share/tmp
rm -r  ${1}/usr/share/zoneinfo
mkdir ${1}/usr/share/zoneinfo
mv ${1}/usr/share/tmp/Europe ${1}/usr/share/zoneinfo

#  -------locales-----------------
mv ${1}/usr/share/i18n/locales/en_US ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/en_GB ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/i18n ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/de_DE@euro ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/de_DE ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/iso14651_t1* ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/locales/translit_* ${1}/usr/share/tmp
rm ${1}/usr/share/i18n/locales/*
mv ${1}/usr/share/tmp/* ${1}/usr/share/i18n/locales/

#  -------charmaps-----------------
mv ${1}/usr/share/i18n/charmaps/ISO-8859-1.gz ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/charmaps/ISO-8859-15.gz ${1}/usr/share/tmp
mv ${1}/usr/share/i18n/charmaps/UTF-8.gz ${1}/usr/share/tmp
rm ${1}/usr/share/i18n/charmaps/*
mv ${1}/usr/share/tmp/* ${1}/usr/share/i18n/charmaps/
rm -r ${1}/usr/share/tmp

#  -------openssh-----------------
rm -fv ${1}/etc/ssl/certs/*
rm -rfv ${1}/usr/share/ca-certificates/*

########################################################################
# löscht alle share directories, die nicht benötigt werden.
########################################################################

rm -r ${1}/usr/share/doc
rm -r ${1}/usr/share/man
rm -r ${1}/usr/share/info
rm -r ${1}/usr/share/sounds
rm -r ${1}/usr/share/misc/pci*

# lirc remotes
rm -r ${1}/usr/share/lirc

rm -r ${1}/usr/share/gettext
rm -r ${1}/usr/share/eselect
########################################################################
# löscht alle librares, die im Basissystem benötigt werden.
# benötigte libs werden über das jeweilige plugin dynamisch geladen
########################################################################

#rm -rv  ${1}/usr/lib/liba52*
#rm -rv  ${1}/usr/lib/libacl*
rm -rv  ${1}/usr/lib/libasprintf*
#rm -rv  ${1}/usr/lib/libattr*
#rm -rv  ${1}/usr/lib/libdca*
#rm -rv  ${1}/usr/lib/libdvdcss*
rm -rv  ${1}/usr/lib/libform*
#rm -rv  ${1}/usr/lib/libgif*
#rm -rv  ${1}/usr/lib/libmagic*
#rm -rv  ${1}/usr/lib/libmodplug*
#rm -rv  ${1}/usr/lib/libnsl*
#rm -rv  ${1}/usr/lib/libogg*
rm -rv  ${1}/usr/lib/libpanel*
#rm -rv  ${1}/usr/lib/libpng*
#rm -rv  ${1}/usr/lib/libpostproc*
#rm -rv  ${1}/usr/lib/libtheora*
#rm -rv  ${1}/usr/lib/libvorbis*
rm -rv  ${1}/usr/lib/libxml2*

for i in `find ${1}/ -name "*.la"`; do
  echo removing: $i
  rm $i
done

for i in `find ${1}/ -name "CVS"`; do
  echo removing: $i
  rm -rf $i
done
