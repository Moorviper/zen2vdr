#!/bin/bash

echo Lint: $1

rm -rf ${1}/etc
rm -rf ${1}/tmp/

rm -rf ${1}/var/lib/samba/*

rm -rf ${1}/usr/lib/samba/
rm -rf ${1}/usr/lib/pkgconfig
rm -rf ${1}/usr/lib/*.la
rm -rf ${1}/usr/lib/libsmbclient*
rm -rf ${1}/usr/share/
rm -rf ${1}/usr/bin/