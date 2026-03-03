//
// FLTK custom font loader
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
	LOG("Load font '" << fontName_ << "' from file " << filePath_ << "\n");
#ifdef _WIN32
	success = (AddFontResourceEx(filePath_, FR_PRIVATE, 0) != 0);
#elif __APPLE__
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)filePath_, strlen(filePath_), false);
	success = CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, NULL);
	if(url) CFRelease(url);
#else
	success = FcConfigAppFontAddFile(NULL, (const FcChar8*)filePath_);
#endif

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
	char path[] = "/tmp/fltk_f_XXXXXX";
	int fd = mkstemp(path);
	if (fd != -1)
	{
		write(fd, data_, len_);
		close(fd);
		success = FcConfigAppFontAddFile(NULL, (const FcChar8*)path);
	}
#endif

	return setupFont(success, fontName_, defaultFont_);
}

//
// Map font to FLTK font index on success, or return defaultFont on error
//
/*static*/
Fl_Font FontLoader::setupFont(bool success_, const char* fontName_, Fl_Font defaultFont_)
{
	if (!success_)
	{
		WNG("Failed to load custom font '" << fontName_ << "'");
		return defaultFont_;
	}
	Fl_Font index = _index;
	Fl::set_font(index, fontName_);
	_index++;
	return index;
}
