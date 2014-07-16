#!/bin/bash

echo Cleaning \(be patient....\)
echo Cleaning OS.
cd ./zentoo-os-build
./clean.sh
cd ..

echo Cleaning tools.
cd ./zentoo-smt-tools
./clean.sh
cd ..

echo Cleaning VDR.
cd ./zenvdr-build
./clean.sh
cd ..

if [ X$1 != Xquick ]; then
  echo Cleaning kernel. \(takes long.\)
  cd ./zentoo-kernel-build
  ./clean.sh
  cd ..
fi

echo Cleaning init files and images.
cd ./zenvdr-init 
./clean.sh
cd ..

echo Done.
