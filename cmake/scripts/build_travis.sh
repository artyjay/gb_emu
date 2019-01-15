#!/bin/bash
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
mkdir build_cmake &&
cd build_cmake &&
cmake .. &&
cmake --build .
