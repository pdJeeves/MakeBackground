#include "depthfile.h"
#include "backgroundexception.h"
#include "png_file.h"
#include "blurheightmap.h"
#include "blur_config.h"
#include <algorithm>
#include <iostream>

DepthFile::DepthFile()
{
	ReadPlatformHeader();
}

void DepthFile::clear()
{
	height.reset();
	platform_mask.reset();
	platformMaskMask &= 0xF0;
}

size_t DepthFile::GetOffset(int mip) const
{
	return (mip > 0) * (size.x * size.y)
		 + (mip > 1) * (size.x * size.y) / 4
		 + (mip > 2) * (size.x * size.y) / 16
	     + (mip > 3) * (size.x * size.y) / 64;
}

uint8_t * DepthFile::GetPlatform(int mip)
{
	if(platform == nullptr)
		ReadPlatform();

	platformMaskMask |= 0x10;
	if(platformMaskMask & (0x10 << mip))
		return &platform[GetOffset(mip)];

	platformMaskMask |= (0x10 << mip);

	uint8_t * dst = &platform[GetOffset(mip)];
	uint8_t * src = GetPlatform(mip-1);

	size_t width  = size.x >> mip;
	size_t height = size.y >> mip;

	std::unique_ptr<uint32_t[]> temp(new uint32_t[width*height]);

//copy into temp;
	for(uint32_t y = 0; y < height; ++y)
	{
		for(uint32_t x = 0; x < width; ++x)
		{
			((uint8_t*) &temp[y*width+x])[0] = src[y*width*4 + x*2];
			((uint8_t*) &temp[y*width+x])[1] = src[y*width*4 + x*2+1];
		}

		for(uint32_t x = 0; x < width; ++x)
		{
			((uint8_t*) &temp[y*width+x])[2] = src[(y*2+1)*width*2 + x*2];
			((uint8_t*) &temp[y*width+x])[3] = src[(y*2+1)*width*2 + x*2+1];
		}
	}

	for(uint32_t y = 0; y < height; ++y)
	{
		for(uint32_t x = 0; x < width; ++x)
		{
			uint8_t * src = (uint8_t*) &temp[y*width+x];

//if all the same use that.
			if(src[0] == src[1]
			&& src[1] == src[2]
			&& src[2] == src[3])
			{
				dst[y*width+x] = src[0];
				continue;
			}

//set to backmost
			dst[y*width+x] = std::min(std::min(src[0], src[1]), std::min(src[2], src[3]));

//get counts
			uint8_t count[4]{0, 0, 0, 0};

			for(int i = 0; i < 4; ++i)
			{
				count[0] += (src[0] == src[i]);
				count[1] += (src[1] == src[i]);
				count[2] += (src[2] == src[i]);
				count[3] += (src[3] == src[i]);
			}

//if one of the components is only 1 thing; then we know we don't use it...
			if((count[0] == 1) || (count[1] == 1) || (count[2] == 1) || (count[3] == 1))
			{
				for(int i = 0; i < 4; ++i)
				{
					if(count[i] >= 2)
					{
						dst[y*width+x] = src[i];
						break;
					}
				}
			}
		}
	}

	return dst;
}

const uint32_t * DepthFile::GetPlatformMask(int mip)
{
	assert(mip < 3);

	size_t start = GetOffset(mip) / 4;

	if(platformMaskMask & (1 << mip))
		return &platform_mask[start];

	platformMaskMask |= (1 << mip);

	if(platform_mask == nullptr)
	{
		platform_mask.reset(new uint32_t[GetOffset(4) / 4]);
		memset(&platform_mask[0], 0, GetOffset(4));
	}

	uint32_t * mask = &platform_mask[start];
	const uint8_t * src = GetPlatform(mip);
	const uint8_t * dst = GetPlatform(mip+1);

	int width  = size.x >> (mip+1);
	int height = size.y >> (mip+1);

//first vertically...
	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			auto    p = dst[y*width + x];
			uint8_t r = 0;

			for(int j = -2; j < +2; ++j)
			{
				if(0 <= y+j && y+j < height)
					r |= (dst[(y+j)*width + x] == p) << (j + 2)*2;
			}

			r |= r << 1;
			mask[y*width+x] = (r | (r << 8)) << 16;
		}
	}

//now horizontally...
	for(int y = 0; y < height; ++y)
	{
		for(int i = 0; i < 2; ++i)
		{
			uint32_t y0 = y * 2 + i;

			for(int x = 0; x < width; ++x)
			{
				auto    p = dst[y*width + x];
				uint8_t r = 0;

				for(int j = -3; j <= 4; ++j)
				{
					if(0 <= x*2+j && x*2+j < width*2)
						r |= (src[y0*(width*2) + x*2+j] == p) << (j + 3);
				}

				mask[y*width+x] |= r << (8*i);
			}
		}
	}


	return &platform_mask[start];
}

void DepthFile::WriteDepth(PngFile & r, float limit)
{
	Load();

	r.size       = size;
	r.color_type = PngFile::ColorType::RGB;
	r.bit_depth  = 8;
	r.channels   = 3;
	r.bytesPerRow = 3 * size.x;

	r.Alloc();

	for(int y = 0; y < size.y; ++y)
	{
		uint8_t * dst = r.row_pointers[y];

		for(int x = 0; x < size.x; ++x)
			dst[x * 3 + 0] = platform[y*size.x + x];
	}

	const float mul = 1.f / limit;

	for(int y = 0; y < size.y; ++y)
	{
		uint8_t * dst = r.row_pointers[y];

		for(int x = 0; x < size.x; ++x)
		{
			float v = height[y*size.x + x] - dst[x*3 + 0];

			dst[x * 3 + 1] = std::min(255u, (uint32_t) (v * mul));
			dst[x * 3 + 2] = 0;
		}
	}

	r.Write();
}

void DepthFile::CopyPlatform(PngFile & file)
{
	if(file.row_pointers == nullptr)
		file.Read();

	if(file.row_pointers == nullptr)
		throw BackgroundException("Unable to load image");

	if(size.x != 0 || size.y != 0)
	{
		if(size.x != file.width() || size.y != file.height())
			throw BackgroundException("Platform map size does not match previous depth data");
	}

	size.x = file.width();
	size.y = file.height();

	if((file.width() & 0xFF) != 0
	|| (file.height() & 0xFF) != 0)
	{
		throw BackgroundException("Platform map width/height must be multiples of 256");
	}

	if(file.bit_depth != 8)
	{
		throw BackgroundException("Platform map must have a bit depth of 8");
	}

	if(platform == nullptr)
		platform.reset(new uint8_t[size.x * size.y * 3 / 2]);

//copy in...
	for(int y = 0; y < size.y; ++y)
	{
		auto src = file.row_pointers[y];
		auto dst = &platform[y * size.x];

		for(int x = 0; x < size.x; ++x)
			dst[x] = src[x*file.channels];
	}
}

void DepthFile::ReadPlatform()
{
	if(platform != nullptr)
		return;

	PngFile png("Platform.png", bg_Type::Depth, 0);

	if(!png.doesExist()
	&& !png.ChangePath("Depth.png")
	&& !png.ChangePath("Depth.gen.png"))
		throw BackgroundException("Unable to locate platform map");

	CopyPlatform(png);
	modified = png.modified;
}

void DepthFile::ReadPlatformHeader()
{
	if(platform != nullptr
	|| size.x > 0 && size.y > 0)
		return;

	PngFile png("Platform.png", bg_Type::Depth, 0);

	if(!png.doesExist()
	&& !png.ChangePath("Depth.png")
	&& !png.ChangePath("Depth.gen.png"))
		throw BackgroundException("Unable to locate platform map");

	png.ReadHeader();
	size = png.size;
	modified = png.modified;

	CheckDepth("Height");
	CheckDepth("Detail");
}

PngFile DepthFile::LocateDepth(std::string const& name)
{
	std::string p(name);
	PngFile png(name, bg_Type::Depth, 0);

	if(png.ChangePath(p + "-16.png"))
		return png;

	PngFile  raw(p + ".png", bg_Type::Depth, 0);

	FileBase config(p + "-Config.json");
	png.ChangePath(p + "-16.gen.png");

	if(png.moreRecent(raw)
	&& png.moreRecent(config))
		return png;

	if(!raw.doesExist())
		throw BackgroundException(std::string("Unable to locate ") + p + " map");

	raw.ReadHeader();

	if(raw.bit_depth == 16)
		return raw;

	std::cout << "Generating " << name << "-16.gen.png (this can take a half hour)" << std::endl;

	auto stages = ReadConfiguration(config.path);
	BlurHeightMap(png, raw, *this, stages);
	png.Write();
	return png;
}

void DepthFile::CheckDepth(const char * name)
{
	PngFile png = LocateDepth(name);
	modified = std::max(png.modified, modified);
}

void DepthFile::AddDepth(const char * name, float multiple)
{
	std::string p(name);

	PngFile png = LocateDepth(p);
	png.Read();

	modified = std::max(png.modified, modified);

	if(png.width() != size.x
	|| png.height() != size.y)
	{
		throw BackgroundException(std::string("Dimensions of: '") + p + "' do not match that of the platform map");
	}

	if(png.bit_depth == 16)
	{
		multiple = multiple / (float) 0x00000100;

		for(int y = 0; y < png.height(); ++y)
		{
			uint16_t * src = (uint16_t*) (png.row_pointers[y]);

			for(int x = 0; x < png.width(); ++x)
			{
				auto v = src[x*png.channels + 0];
				v = (v << 8) | (v >> 8);

				height[y*png.width() + x] += (v * multiple);
			}
		}
	}
	else if(png.bit_depth == 8)
	{
		if(png.channels < 3)
		{
			throw BackgroundException(png.path + std::string(": 8 bit '-16' maps must be in RGB color space!"));
		}

		float mul_g = multiple;
		float mul_b = multiple / (float) 0x00000100;

		for(int y = 0; y < png.height(); ++y)
		{
			uint16_t * src = (uint16_t*) png.row_pointers[y];

			for(int x = 0; x < png.width(); ++x)
			{
				height[y*png.width() + x]
					+= src[x*png.channels + 1] * mul_g
					+  src[x*png.channels + 2] * mul_b;
			}
		}
	}
	else
	{
		throw BackgroundException(png.path + std::string(": not a 16 bit image."));
	}


}


void DepthFile::Load()
{
	if(height != nullptr)
		return;

	ReadPlatform();

//--------------------------
// create height and set to platform...
//--------------------------
	height.reset(new float[size.x * size.y]);

	memset(&height[0], 0, sizeof(float) * size.x * size.y);

	const int N = size.x * size.y;
	for(int i = 0; i < N; ++i)
		height[i] = platform[i];

//--------------------------
// Get 16 bit depth...
//--------------------------
	AddDepth("Height", 4);
	AddDepth("Detail", 1);
}
