#!/bin/bash
mkdir -p build
cd build
rm -rf *
export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
cd ..
echo "Executable in build/"
