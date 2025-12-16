#!/bin/sh

APPLICATION=fltk-schnapsen

cat $APPLICATION.desktop | envsubst >~/.local/share/applications/$APPLICATION.desktop
chmod 0777 ~/.local/share/applications/$APPLICATION.desktop
cp -v deck.gif ~/.$APPLICATION/rsc/.
cp -v *.mp3 ~/.$APPLICATION/rsc/.
cp -v $APPLICATION.svg ~/.$APPLICATION/rsc/.
