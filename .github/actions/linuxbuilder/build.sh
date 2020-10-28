#!/bin/sh

mkdir build && pushd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --config Release
make install DESTDIR=dist
