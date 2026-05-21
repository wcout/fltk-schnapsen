//
// Part of "Schnapsen for 2" card game.
//
// (c) 2026 Christian Grabner
//
// FLTK custom font loader.
//

#include "FontLoader.h"
#include "debug.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#else
#include <fontconfig/fontconfig.h>
#include <cstdlib>
#include <unistd.h>
#endif
#include <filesystem>

// First index for custom fonts
/*static*/
Fl_Font FontLoader::_index = FL_FREE_FONT;

//
// Load font from file `filePath_`, use defaultFont on failure
//
/*static*/
Fl_Font FontLoader::load(const char* filePath_, const char* fontName_, Fl_Font defaultFont_/* = FL_HELVETICA*/)
{
	bool success = false;
	if (std::filesystem::exists(filePath_))
	{
		LOG("Load font '" << fontName_ << "' from file " << filePath_ << "\n");
#ifdef _WIN32
		success = (AddFontResourceEx(filePath_, FR_PRIVATE, 0) != 0);
#elif __APPLE__
		CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, reinterpret_cast<const UInt8*>(filePath_), strlen(filePath_), false);
		success = CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, NULL);
		if (url) CFRelease(url);
#else
		success = FcConfigAppFontAddFile(NULL, reinterpret_cast<const FcChar8*>(filePath_));
#endif
	}
	return setupFont(success, fontName_, defaultFont_);
}

//
// Load font from memory, use default font on failure
//
/*static*/
Fl_Font FontLoader::load(const unsigned char* data_, unsigned int len_, const char* fontName_, Fl_Font defaultFont_/* = FL_HELVETICA*/)
{
	bool success = false;
	LOG("Load embedded font '" << fontName_ << "'\n");
#ifdef _WIN32
	DWORD n;
	HANDLE h = AddFontMemResourceEx((void*)data_, len_, NULL, &n);
	success = (h != NULL);
#elif __APPLE__
	CFDataRef d = CFDataCreate(NULL, data_, len_);
	CGDataProviderRef p = CGDataProviderCreateWithCFData(d);
	CGFontRef f = CGFontCreateWithDataProvider(p);
	success = CTFontManagerRegisterGraphicsFont(f, NULL);
	CFRelease(f); CFRelease(p); CFRelease(d);
#else
	// Linux: Use a temporary file for the memory data
	char path[] = "/tmp/fltk_font_XXXXXX";
	int fd = mkstemp(path);
	if (fd != -1)
	{
		success = write(fd, data_, len_) == len_;
		close(fd);
		success = success && FcConfigAppFontAddFile(NULL, reinterpret_cast<const FcChar8*>(path));
	}
#endif

	return setupFont(success, fontName_, defaultFont_);
}

//
// Add all fonts from directory to font list
//
/*static*/
int FontLoader::load_dir(const char* dir_path_, const char *ext_/* = ".ttf"*/)
{
	std::filesystem::path fonts(dir_path_);
	int n = 0;
	for (auto const &dir_entry : std::filesystem::directory_iterator(fonts))
	{
		if (dir_entry.is_regular_file() && dir_entry.path().extension().string() == ext_)
		{
			n += (int)load(dir_path_, dir_entry.path().filename().string().c_str(), (Fl_Font)-1);
		}
	}
	DBG("FontLoader::load_dir(" << dir_path_ << "): added " << n << " fonts\n");
	return n;
}

//
// Map font to FLTK font index on success, or return defaultFont on error
//

static std::string fontName;

/*static*/
Fl_Font FontLoader::setupFont(bool success_, const char* fontName_, Fl_Font defaultFont_)
{
	if (!success_)
	{
//		WNG("Failed to load custom font '" << fontName_ << "'");
//		return defaultFont_;
		// continue in normal font setting...
	}
	if (defaultFont_ == (Fl_Font)-1)
	{
		DBG("added custom font " << fontName_ << "\n");
		return (Fl_Font)success_;
	}

	// We have to use the font name as FLTK presents us
	fontName = fontName_;
	int n = Fl::set_fonts();
	DBG(n << " fonts in FLTK font table\n");
	int use_font = -1;
	DEV("---------\t");
	for (int i = 0; i < n; i++)
	{
		std::string fn = Fl::get_font_name(i);
		DEV(i << "\t" << fn << "\n");
	}
	DEV("---------\n");

	for (int i = FL_FREE_FONT; i < n; i++)
	{
		std::string fn = Fl::get_font_name(i);
		DEV("\t[" << fontName << "].startsWith(" << fn << ")\n");
		if (!fontName.starts_with(fn)) continue;
		if (use_font < 0)
		{
			use_font = i;
		}
		DEV(i << ": test fontname '" << fn << "' against '" << fontName << "'\n");
		if (fn.starts_with(fontName_))
		{
			use_font = i;
			DEV("break with " << fn << "\n");
			break;
		}
	}

	Fl_Font index = _index;
	if (use_font >=0 )
	{
		LOG("Use font #" << use_font << ": '" << Fl::get_font(use_font) << "'\n");
		Fl::set_font(index, use_font);
	}
	else
	{
		WNG("Failed to set font '" << fontName << "'");
		return defaultFont_;
#if 0
		// not found, try to set by name anyway
		LOG("Set font: '" << fontName << "'\n");
		const char *default_font = Fl::get_font(FL_HELVETICA);
		DBG("default font: '" << default_font << "'\n");
		if (default_font[0] == ' ') fontName = " " + fontName;
		Fl::set_font(index, fontName.c_str());
#endif
	}
	_index++;
	return index;
}

//
// Make font name from font file e.g. "name_subname-Regular.ttf" => "name subname Regular"
//
/*static*/
std::string FontLoader::convertToFontName(const std::string &name_)
{
	std::string name(name_);
	size_t pos;
	while ((pos = name.find('-')) != std::string::npos)
	{
		name[pos] = ' ';
	}
	while ((pos = name.find('_')) != std::string::npos)
	{
		name[pos] = ' ';
	}
	if ((pos = name.rfind('.')) != std::string::npos)
		name.erase(pos);
	return name;
}
