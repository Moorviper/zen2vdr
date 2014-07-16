#!/bin/bash

BASEDIR=`pwd`

cd linux-2.6.36-aufs
cp .config ../linux-2.6.36-aufs.config
make clean
cd $BASEDIR

tar cjf - linux-2.6.36-aufs | split -b 32M -a 1 - linux-2.6.36-aufs.tbz.part
