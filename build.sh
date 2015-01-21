#!/bin/bash

BASEDIR=`pwd`

cd zentoo-kernel-build
./build.sh
cd $BASEDIR

cd zentoo-os-build
./build.sh
cd $BASEDIR

cd zentoo-smt-tools
PREFIX="/zensysgen-glibc" ./build.sh
cd $BASEDIR

cd zenvdr-build
RESOURCE="R3" TARGET_DIR="/zensysgen-glibc" METHOD="tar" ./build.sh
cd $BASEDIR

cd zenvdr-init
./install.sh "/zensysgen-glibc"
cd $BASEDIR
