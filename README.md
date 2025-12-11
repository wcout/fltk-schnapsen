# FLTK Schnapsen

Schnapsen is an implementation of the popular (Austrian) card game ["Schnapsen"](https://en.wikipedia.org/wiki/Schnapsen) for [FLTK](https://www.fltk.org/).
You play against the computer.

![Screenshot of FLTK Schnapsen](rsc/fltk-schnapsen.png "Screenshot of FLTK Schnapsen")


It is developed for Linux, but should theoretically run also on Windows or macOS.

## Dependencies

Only FLTK version 1.4 or higher.

## Build

Basically just uses `fltk-config` to build.
Makefile or CMakeLists.txt are supplied.
Uses C++ standard 20!

## Running the program

Just run from its folder or - under Linux - use the `install.sh` script to place it in local user directories.

The program can use German or English as its language. It uses the system "locale" to see if it can use German, otherwise it switches to English.

You can run it fullscreen by pressing `F10`. It will memorize that in it's config file `fltk-schnapsen.cfg`.

You can also supply a background tile graphic (image) by command line using `--background {path to image}` or can use a color from FLTK's standard color palette with `--background {number}`.

## Game control

Fetch the card you want to play by clicking on it, then move it (without mouse button pressed) to the center of the "table" and click again to play it.
If you re-consider your decision while still moving the card click the right button to return it into your hand.

Use the mouse to declare a "marriage" by dragging your queen in your hand over the corresponding king and click to announce (and play).

Drag the matching Jack in your hand over the trump card under the card deck and click to "grab" the trump card.

Click on the deck to "close" the game.

Move the mouse over your played pack to see the tricks. You can also see the first trick of the computer by hoovering the mouse over its pack.

## Status

Still in development.

Game flow and graphics are quite complete.

Game play of the AI is very basic, but not completely foolish. Will be improved over time.

## Varia

The game uses licence free SVG card images from various sources, in particular from:

[SVG playing cards](https://commons.wikimedia.org/wiki/Category:SVG_playing_cards)
