#!/bin/sh

cmake -S . -B build -DCMAKE_INSTALL_PREFIX:PATH=/usr
make -C build
make -C build install DESTDIR=dist
