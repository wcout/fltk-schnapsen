#!/bin/sh

APPLICATION=fltk-schnapsen

cp -v $APPLICATION ~/bin/.
mkdir -p ~/.$APPLICATION/rsc
cp -av svg_cards ~/.$APPLICATION/.
cd rsc
sh install.sh
cd ..
