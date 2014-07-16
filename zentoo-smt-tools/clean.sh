#!/bin/bash

BASEDIR=`pwd`

cd smtlircd
make clean
cd $BASEDIR

cd smt-ready
make clean
cd $BASEDIR

cd fs454tool
make clean
cd $BASEDIR

cd stv6421tool
make clean
cd $BASEDIR
