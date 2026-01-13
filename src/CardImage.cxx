#include "CardImage.h"

#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_PNG_Image.H>

#include <cassert>

Fl_RGB_Image *rotate_90_CCW(const Fl_RGB_Image &svg_)
{
	int w = svg_.data_w();
	int h = svg_.data_h();
	DBG("rotate " << w << "x" << h << "\n");
	int d = svg_.d();
	assert(w > 0 && h > 0 && d >= 3);
	uchar alpha = 0;
	uchar *rot_data = new uchar[w * h * d];

	const auto data = svg_.data()[0];
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			auto get_offset = [](int w, [[maybe_unused]]int h, int x, int y, int d) -> size_t
			{
				return w * y * d + x * d;
			};
			size_t offset = get_offset(w, h, x, y, d);
			unsigned char r = data[offset];
			unsigned char g = data[offset + 1];
			unsigned char b = data[offset + 2];
			if (d > 3)
				alpha = data[offset + 3];

			offset = get_offset(h, w, h - y - 1, x, d);
			rot_data[offset]     = r;
			rot_data[offset + 1] = g;
			rot_data[offset + 2] = b;
			if (d > 3)
				rot_data[offset + 3] = alpha;
		}
	}
	Fl_RGB_Image *rotated_image = new Fl_RGB_Image(rot_data, h, w, d);
	rotated_image->alloc_array = 1;
	return rotated_image;
}

CardImage& CardImage::image(const std::string &id_, const std::string &pathname_)
{
	assert(_images.find(id_) == _images.end());
	std::string pathname(pathname_);
	Fl_RGB_Image *svg = new Fl_SVG_Image(pathname.c_str());
	if ((!svg || svg->w() <= 0 || svg->h() <= 0))
	{
		delete svg;
		pathname.erase(pathname.size() - 3);
		pathname.append("png");
		svg = new Fl_PNG_Image(pathname.c_str());
	}
	if ((!svg || svg->w() <= 0 || svg->h() <= 0))
	{
		fl_alert("Card image '%s' not found!", pathname_.c_str());
		exit(EXIT_FAILURE);
	}
	assert(svg && svg->w() > 0 && svg->h() > 0);
	if (svg->as_svg_image())
		svg->as_svg_image()->proportional = 0;
	_images[id_] = svg;
	_last_id = id_;
	return *this;
}

Fl_RGB_Image *CardImage::image(const std::string &id_, int w_/* = 0*/, int h_/* = 0*/) const
{
	auto image = _images.find(id_);
	if (image == _images.end())
		return nullptr;
	image->second->scale(w_ ? w_ :_W , h_ ? h_ : _H, 0, 1);
	return image->second;
}

Fl_RGB_Image *CardImage::image() const
{
	assert(_last_id.size());
	return image(_last_id);
}

Fl_RGB_Image *CardImage::skewed_image(const std::string &id_)
{
	Fl_RGB_Image *svg = image(id_);
	assert(svg && svg->w() > 0 && svg->h() > 0);
	assert(_W != 0 && _H != 0);
	Fl_RGB_Image *skewed_image = _images[id_ + "_skewed"];
	if (svg->as_svg_image())
		svg->as_svg_image()->resize(_W, _H);
	if (!skewed_image
		|| skewed_image->w() != _W || skewed_image->h() != _H / 3)
	{
		delete skewed_image;
		skewed_image = static_cast<Fl_RGB_Image *>(svg->copy());
		_images[id_ + "_skewed"] = skewed_image;
	}
	assert(skewed_image);
	skewed_image->scale(_W, _H / 3, 0, 1);
	return skewed_image;
}

Fl_RGB_Image *CardImage::quer_image(const std::string &id_)
{
	Fl_RGB_Image *svg = image(id_);
	assert(svg && svg->w() > 0 && svg->h() > 0);
	assert(_W != 0 && _H != 0);
	Fl_RGB_Image *quer_image = _images[id_ + "_quer"];
	if (svg->as_svg_image())
		svg->as_svg_image()->resize(_W, _H);
	if (!quer_image
		|| quer_image->w() != _H || quer_image->h() != _W)
	{
		delete quer_image;
		quer_image = rotate_90_CCW(*svg);
		_images[id_ + "_quer"] = quer_image;
		DBG("rotate image '" << id_ << "' size (before scale): " << quer_image->w() << "x" << quer_image->h() << "\n");
	}
	assert(quer_image);
	quer_image->scale(_H, _W, 0, 1);
	return quer_image;
}

Fl_RGB_Image *CardImage::quer_image()
{
	assert(_last_id.size());
	return quer_image(_last_id);
}

Fl_RGB_Image *CardImage::skewed_image()
{
	assert(_last_id.size());
	return skewed_image(_last_id);
}

CardImage& CardImage::set_pixel_size(int w_, int h_)
{
	_W = w_;
	_H = h_;
	return *this;
}

/*static*/ int CardImage::_W = 0;
/*static*/ int CardImage::_H = 0;
/*static*/std::unordered_map<std::string, Fl_RGB_Image *> CardImage::_images;
