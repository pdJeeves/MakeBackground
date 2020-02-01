#ifndef LINEARIMAGE_H
#define LINEARIMAGE_H
#include "bg_type.h"
#include "png_file.h"
#include <glm/vec2.hpp>
#include <memory>

class PngFile;

class LinearImage
{
public:
	LinearImage() = default;
	~LinearImage() = default;

	void toLinear();
	void fromLinear();

	int width() const { return size.x; }
	int height() const { return size.y; }

	glm::ivec2 size;
	int channels{1};

	bg_Type type{};
	PngFile::ColorType color_type{};

	std::unique_ptr<float[]> data;

	void toPng(PngFile & dst) const;
	void fromPng(PngFile & src);

	void LinearDownscale(const uint32_t * platform_mask, float * kernel, int size);
};

#endif // LINEARIMAGE_H
