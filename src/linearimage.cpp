#include "linearimage.h"
#include "png_file.h"
#include <glm/glm.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

void LinearImage::toPng(PngFile & png) const
{
	png.size         = size;
	png.color_type   = PngFile::ColorType::RGB;
	png.bit_depth    = 8;
	png.channels     = channels;
	png.bytesPerRow  = channels * width();
	png.color_type   = color_type;

	png.Alloc();

	for(int y = 0; y < height(); ++y)
	{
		auto      dst = (png.row_pointers[y]);
		float   * src = &data[y*width()*channels];

		for(int x = 0; x < width()*channels; ++x)
		{
			dst[x] = (uint8_t) std::max(0, std::min<int>(src[x] * 255.f, 255));
		}
	}
}

void LinearImage::fromPng(PngFile & png)
{
	png.Read();

	size       = png.size;
	channels   = png.channels;
	type       = png.type;
	color_type = png.color_type;

	data.reset(new float[width()*height()*channels]);

	for(int y = 0; y < height(); ++y)
	{
		auto      src = (png.row_pointers[y]);
		float   * dst = &data[y*width()*channels];

		for(int x = 0; x < width()*channels; ++x)
		{
			dst[x] = src[x] / 255.f;
		}
	}
}

void LinearImage::LinearDownscale(const uint32_t * platform_mask, float * kernel, int length)
{
	std::unique_ptr<float[]> scratch(new float[width()*height()*5/2]);
	memset(&scratch[0], 0, width()*height()*5/2);

	int w = width()/2;
	int h = height()/2;

//scale in X direction
	for(int y = 0; y < height(); ++y)
	{
		int y_mask = (y & 1) * 8;

		for(int x = 0; x < width(); x += 2)
		{
			float * px = &scratch[(y*w + x/2)*5];

			int x0 = (x+1) - (length)/2;
			uint16_t p = platform_mask[(y/2)*w+x/2] & 0x0000FFFFF;

			p = 0xFF & (p >> y_mask);
			p >>= (8 - length)/2;

			for(int i = 0; i < length; ++i)
			{
				if(0 <= x0+i && x0+i < width()
				&& ((p & (1 << i)) || kernel[i] < 0))
				{
					for(int c = 0; c < channels; ++c)
						px[c] += data[(y*width()+ x0+i)*channels+c] * kernel[i];

					px[4] += kernel[i];
				}
			}

			if(px[4] <= 0)
				memset(&px[0], 0, sizeof(float) * 5);
		}
	}

	data.reset(new float[width()*height()*channels/4]);

//scale in Y direction...
	for(int y = 0; y < height(); y += 2)
	{
		for(int x = 0; x < w; ++x)
		{
			float px[5]{0, 0, 0, 0, 0};

			int y0 = (y+1) - (length-2)/2;

			uint16_t p = (platform_mask[(y/2)*w+x] >> 16) & 0xFF;

			p >>= (8 - length)/2;

			for(int i = 0; i < length; ++i)
			{
				if(0 <= y0+i && y0+i < height()
				&& ((p & (1 << i)) || kernel[i] < 0))
				{
					for(int c = 0; c < 4; ++c)
						px[c] += scratch[((y0+i)*w+ x)*5+c] * kernel[i];

					px[4] += std::fabs(scratch[((y0+i)*w+ x)*5+4]) * kernel[i];
				}
			}

			if(px[4] <= 0)
				memset(&data[((y/2)*w + x)*channels], 0, sizeof(float) * channels);
			else
			{
				for(int c = 0; c < channels; ++c)
				{
					data[((y/2)*w + x)*channels+c] = px[c]/px[4];
				}
			}
		}
	}

	size = glm::ivec2(w, h);
}

void LinearImage::toLinear()
{

	switch(type)
	{
	case bg_Type::Diffuse:
	{
		uint32_t chn = std::max(3, channels);
		uint32_t N   = height() * width() * channels;

		for(uint32_t i = 0; i < N; i += channels)
		{
			for(uint32_t c = 0; c < chn; ++c)
			{
				data[i + c] = std::pow(data[i + c], 2.2f);
			}
		}
	}
	case bg_Type::Depth:
		break;
	case bg_Type::Normals:
	{
		uint32_t N = height() * width() * channels;

		for(uint32_t i = 0; i < N; ++i)
		{
			data[i] = data[i] * 2.f - 1.f;
		}
	}
		break;
	case bg_Type::Roughness:
		break;
	case bg_Type::Occlusion:
	break;
	default:
		break;
	}
}

void LinearImage::fromLinear()
{
	switch(type)
	{
	case bg_Type::Diffuse:
	{
		uint32_t chn = std::max(3, channels);
		uint32_t N   = height() * width() * channels;

		for(uint32_t i = 0; i < N; i += channels)
		{
			for(uint32_t c = 0; c < chn; ++c)
			{
				data[i + c] = std::pow(data[i + c], 1.f/2.2f);
			}
		}
	}
	case bg_Type::Depth:
		break;
	case bg_Type::Normals:
	{
		uint32_t N = height() * width() * channels;

		for(uint32_t i = 0; i < N; i += 3)
		{
			glm::vec3 v = glm::vec3(data[i+0], data[i+1], data[i+2]);
			v = glm::normalize(v);
			v = (v + 1.f) / 2.f;

			data[i+0] = v[0];
			data[i+1] = v[1];
			data[i+2] = v[2];
		}
	}
		break;
	case bg_Type::Roughness:
		break;
	case bg_Type::Occlusion:
	break;
	default:
		break;
	}
}
