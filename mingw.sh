#!/bin/sh

fltkconfig=../../fltk/build_mingw32/fltk-config

cxx=`$fltkconfig --cxx`

#custom_font=-DCUSTOM_FONT=\"Mystery_Quest-Regular.ttf\"
custom_font=-DCUSTOM_FONT=\"DynaPuff-Regular.ttf\"

$cxx -o fltk-schnapsen.exe -g -Wall -pedantic -Isrc -Iinclude $custom_font -DUSE_IMAGE_TEXT -DUSE_MINIAUDIO -std=c++20 `$fltkconfig --use-images --cxxflags` fltk-schnapsen.cxx `$fltkconfig --use-images --ldstaticflags` -static -lgcc -static -lstdc++ -static -lwinmm
