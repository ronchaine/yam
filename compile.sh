#!/bin/sh

cd build
CXX=clang++ cmake ../deps/wheel/
make
cd ..

echo "compiling test program..."

clang++  -std=c++11 -g game.cpp deps/wheel-extras/font/font.cpp \
         console.cpp \
         -pthread -Ideps/wheel/include \
         -fno-rtti -fno-exceptions \
         -Ideps/wheel-extras/include \
         -Ideps/Selene/include \
         `freetype-config --cflags --libs` \
         `sdl2-config --cflags --libs` \
         -llua \
         -Lbuild/src -lwheel_static -lGLEW -lGL -lphysfs
