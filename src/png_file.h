#ifndef PNG_FILE_H
#define PNG_FILE_H
#include "filebase.h"
#include "bg_type.h"
#include <glm/vec2.hpp>
#include <memory>
#include <png.h>

class DepthFile;

class PngFile : public FileBase
{
public:
	enum class ColorType : png_byte
	{
		GRAY = PNG_COLOR_TYPE_GRAY,
		GRAY_ALPHA = PNG_COLOR_TYPE_GRAY_ALPHA,
		PALETTE = PNG_COLOR_TYPE_PALETTE,
		RGB = PNG_COLOR_TYPE_RGB,
		RGBA = PNG_COLOR_TYPE_RGB_ALPHA,

		PALETTE_MASK = PNG_COLOR_MASK_PALETTE,
		COLOR_MASK = PNG_COLOR_MASK_COLOR,
		ALPHA_MASK = PNG_COLOR_MASK_ALPHA,
	};

	PngFile(std::string && path, bg_Type type, int mip) : FileBase(std::move(path)), type(type), mip(mip) {}
	PngFile(std::string const& path, bg_Type type, int mip) : FileBase(path), type(type), mip(mip) {}
	PngFile(PngFile const&) = delete;
	PngFile(PngFile &&);
	~PngFile();

	void Alloc();
	void Scale(DepthFile & depth, PngFile &);

	void ReadHeader();
	void Read();
	void Write();

	bg_Type type{};
	int     mip{};
	uint32_t bytesPerRow{};

	int width() const { return size.x; }
	int height() const { return size.y; }

	glm::ivec2 size{0, 0};
	ColorType color_type{};
	png_byte bit_depth{};
	png_byte channels{};
	
	png_bytep * row_pointers{nullptr};
	png_bytep   image{};

	void clear();
	void wait();

	std::unique_ptr<png_bytep[]> GetInvRows();

private:
};

#endif // PNG_FILE_H
