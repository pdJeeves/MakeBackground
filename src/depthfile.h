#ifndef DEPTHFILE_H
#define DEPTHFILE_H
#include <glm/gtc/type_precision.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <string>

class PngFile;

class DepthFile
{
public:
	DepthFile();

	void Load();
	void clear();

	std::string path;
	time_t      modified{};

// red = same platform as us
// green
	std::unique_ptr<uint8_t[]>     platform;
	std::unique_ptr<float[]>       height;
	glm::ivec2                     size{0, 0};

	size_t GetOffset(int mip) const;

	      uint8_t * GetPlatform(int mip);
	const uint32_t * GetPlatformMask(int mip);

	void DownscalePlatform(int mip);

	//either in the red channel or greyscale image
	void ReadPlatform();
	void ReadPlatformHeader();
	void CopyPlatform(PngFile & file);

	//either in green + blue channels or greyscale image
	void AddDepth(const char * name, float multiple);
	void AddDepth(PngFile const& file, float multiple);
	void WriteDepth(PngFile & file, float limit);

private:
	void CheckDepth(const char * name);
	PngFile LocateDepth(const std::string & name);

	std::unique_ptr<uint32_t[]>    platform_mask;
	uint32_t platformMaskMask{};
};

#endif // DEPTHFILE_H
