#!/usr/bin/sh

mkdir -p build_debug
cd build_debug/
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
cd ../
mkdir -p build_release
cd build_release/
cmake -DCMAKE_BUILD_TYPE=Release ../
make
cd ../

