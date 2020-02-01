#ifndef BACKGROUNDLAYER_H
#define BACKGROUNDLAYER_H
#include "backgroundexception.h"
#include <memory>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "ddsfile.h"



void CopyBlock(DXT5_Block * dst, const uint8_t * src, int BC, int block_id);
void CopyBlock(DXT1_Block * dst, const uint8_t * src, int BC, int block_id);
void CopyBlock(BC4_Block * dst, const uint8_t * src, int BC, int block_id);
void CopyBlock(BC5_Block * dst, const uint8_t * src, int BC, int block_id);

class DDSFile;

template<typename BC_ID>
class BackgroundLayer
{
public:
	BackgroundLayer() = default;


	void SetFile(DDSFile & file)
	{
		file.Read();
		modified = file.modified;

		if(file.data.empty())
			return;

		if(file.header.dwWidth % 256 != 0 || file.header.dwHeight % 256 != 0)
		{
			throw DDSException(file.path + ": image width/height not evenly divisible by 256.");
		}

		if(tiles_x == 0 && tiles_y == 0)
		{
			tiles_x = file.header.dwWidth / 256;
			tiles_y = file.header.dwHeight / 256;
		}
		else if(file.header.dwWidth / 256 != tiles_x || file.header.dwHeight / 256 != tiles_y)
		{
			throw DDSException(file.path + ": image size does not match previous files");
		}

		const size_t no_tiles   = length();
		const int    BC         = file.GetBcId();
		const int    stride     = file.GetStride();

		int blocks     = (256 * 256) / 16;

		if(BC == 0)
			throw DDSException(file.path + ": non-BC1-5 compression type");

		uint8_t * src = &file.data[0];

		uint32_t N = std::min<uint32_t>(file.header.dwMipMapCount, 4);

		for(uint32_t i = 0; i < N; ++i)
		{
			CopyTiles(tiles[i], src, 256 >> i, BC);
			src     += length() * stride * blocks;
			blocks >>= 2;
		}

		file.clear();
	}

	uint32_t tiles_x{0};
	uint32_t tiles_y{0};

	size_t length() const { return tiles_x * tiles_y; }

	time_t  modified{0};

	std::unique_ptr<std::unique_ptr<BC_ID[]>[]> tiles[4];
	std::unique_ptr<std::unique_ptr<BC_ID[]>[]> & operator[](int i) { return tiles[i]; }

private:
	void CopyTiles(std::unique_ptr<std::unique_ptr<BC_ID[]>[]> & dst, const uint8_t * src, int tile_size, int BC)
	{
		const int blocks = (tile_size * tile_size) / 16;
		const int tiles  = tiles_x * tiles_y;

		if(dst == nullptr)
			dst = std::unique_ptr<std::unique_ptr<BC_ID[]>[]>(new std::unique_ptr<BC_ID[]>[tiles]);

		const int tile_width = tile_size / 4;
		const int row_width  = tile_width * tiles_x;

		for(int i = 0; i < tiles; ++i)
		{
			const int x_offset = (i % tiles_x) * tile_width;
			const int y_offset = (i / tiles_x) * tile_width;

			if(dst[i] == nullptr)
			{
				dst[i] = std::unique_ptr<BC_ID[]>(new BC_ID[blocks]);
				memset(&dst[i][0], 0, blocks * sizeof(BC_ID));
			}

			for(int j = 0; j < blocks; ++j)
			{
				const int x = (j % tile_width);
				const int y = (j / tile_width);

				CopyBlock((BC_ID*) &dst[i][j], src, BC, (y_offset + y) * row_width + x_offset + x);
			}
		}
	}

};
#endif // BACKGROUNDLAYER_H
