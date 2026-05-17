#pragma once

//
// FLTK custom font loader
//

#include <FL/Fl.H>

class FontLoader
{
public:
	// Load font from file `filePath_`
	static Fl_Font load(const char* filePath_, const char* fontName_, Fl_Font defaultFont_ = FL_HELVETICA);
	// Load font from memory
	static Fl_Font load(const unsigned char* data_, unsigned int len_, const char* fontName_, Fl_Font defaultFont_ = FL_HELVETICA);
	// Add all fonts from directory to font list
	static int load_dir(const char* dirPath_, const char *ext_ = ".ttf");
public:
	// Convert a filename to a font name
	static std::string convertToFontName(const std::string &name_);

private:
	// map font to FLTK font index on success
	static Fl_Font setupFont(bool success_, const char* fontName_, Fl_Font defaultFont_);

private:
	// Current font index, to which loaded font is mapped (starts with FL_FREE_FONT)
	static Fl_Font _index;
};
