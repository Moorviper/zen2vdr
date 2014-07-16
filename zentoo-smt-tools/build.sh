#!/bin/bash

BASEDIR=`pwd`

cd smtlircd
make
make PREFIX=$PREFIX install
cd $BASEDIR

cd smt-ready
make
make PREFIX=$PREFIX install
cd $BASEDIR

cd fs454tool
make
make PREFIX=$PREFIX install
cd $BASEDIR

cd stv6421tool
make
make PREFIX=$PREFIX install
cd $BASEDIR
