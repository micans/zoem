#!/bin/bash

set -euo pipefail

modes=${1?Need mode; c for cimfomfa and/or z for zum}
SOURCE=${2?Please supply directory to grab tarballs from}

cff=21-101
zum=21-101

zumtar=zoem-$zum.tar.gz
cfftar=cimfomfa-$cff.tar.gz

INSTALL=$HOME/phloobah

if [[ $modes =~ c ]]; then
  thedir=${cfftar%.tar.gz}
  rm -rf $thedir
  cp $SOURCE/$cfftar . && tar xzf $cfftar
  ( cd $thedir
    ./configure --prefix=$INSTALL
    make
    make install
  )
fi

if [[ $modes =~ z ]]; then
  cp $SOURCE/$zumtar . && tar xzf $zumtar
  thedir=${zumtar%.tar.gz}
  ( cd $thedir
    ./configure CFLAGS=-I$INSTALL/include LDFLAGS=-L$INSTALL/lib --prefix=$INSTALL
    make
    make install
  )
fi


