#!/bin/sh

cd build
CXX=clang++ cmake ../deps/wheel/
make
cd ..

echo "compiling test program..."

clang++  -std=c++11 game.cpp deps/wheel-extras/font/font.cpp \
         -fno-exceptions -fno-rtti -pthread -Ideps/wheel/include \
         -Ideps/wheel-extras/include \
         `freetype-config --cflags --libs` \
         `sdl2-config --cflags --libs` \
         -Lbuild/src -lwheel_static -lGLEW -lGL -lphysfs
