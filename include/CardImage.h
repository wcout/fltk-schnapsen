#pragma once

#include <FL/Fl_RGB_Image.H>
#include <string>
#include <unordered_map>

class CardImage
{
public:
	CardImage& image(const std::string &id_, const std::string &pathname_);
	Fl_RGB_Image *image(const std::string &id_, int w_ = 0, int h_ = 0) const;
	Fl_RGB_Image *image() const;
	Fl_RGB_Image *skewed_image(const std::string &id_);
	Fl_RGB_Image *rot90_image(const std::string &id_);
	Fl_RGB_Image *rot90_image();
	Fl_RGB_Image *skewed_image();
	CardImage &set_pixel_size(int w_, int h_);
private:
	static int _W;
	static int _H;
	std::string _last_id;
	static std::unordered_map<std::string, Fl_RGB_Image *> _images;
};
