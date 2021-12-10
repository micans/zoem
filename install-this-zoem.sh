#!/bin/bash

set -euo pipefail

# This script is a simple script for downloading + compiling zoem.

cff=21-341
zum=21-341

# Change if you want to install somewhere else
INSTALL=$HOME/local

# Now the rest of this script should have enough to run.

zumtar=zoem-$zum.tar.gz
cfftar=cimfomfa-$cff.tar.gz

if command -v wget > /dev/null; then 
   webbit=wget
elif command -v curl > /dev/null; then 
   webbit="curl -O"
else
   echo "Explain to me how to download stuff please"
   false
fi

$webbit http://micans.org/zoem/src/$zumtar
$webbit http://micans.org/cimfomfa/src/$cfftar

if true; then
  thedir=./${cfftar%.tar.gz}
  rm -rf $thedir
  tar xzf $cfftar
  ( cd $thedir
    ./configure --prefix=$INSTALL
    make
    make install
  )
fi

if true; then
  tar xzf $zumtar
  thedir=./${zumtar%.tar.gz}
  ( cd $thedir
    ./configure CFLAGS=-I$INSTALL/include LDFLAGS=-L$INSTALL/lib --prefix=$INSTALL
    make
    make install
  )
fi

