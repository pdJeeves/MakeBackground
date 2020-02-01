#include "backgroundlayer.h"
#include "ddsfile.h"
#include <cstring>
#include <stdexcept>

void CopyBlock(DXT5_Block * dst, const uint8_t * src, int BC, int block_id)
{
	switch(BC)
	{
	case 1:
		dst->color = ((DXT1_Block*)src)[block_id];
		break;
	case 3:
		*dst = ((DXT5_Block*)src)[block_id];
		break;
	case 4:
		dst->alpha = ((BC4_Block*)src)[block_id];
		break;
	default:
		throw std::runtime_error("bad BC assignment");
	};
}

void CopyBlock(DXT1_Block * dst, const uint8_t * src, int BC, int block_id)
{
	switch(BC)
	{
	case 1:
		*dst = ((DXT1_Block*)src)[block_id];
		break;
	case 3:
		*dst = ((DXT5_Block*)src)[block_id].color;
		break;
	default:
		throw std::runtime_error("bad BC assignment");
	};
}

void CopyBlock(BC4_Block * dst, const uint8_t * src, int BC, int block_id)
{
	switch(BC)
	{
	case 3:
		*dst = ((DXT5_Block*)src)[block_id].alpha;
		break;
	case 4:
		*dst = ((BC4_Block*)src)[block_id];
		break;
	default:
		throw std::runtime_error("bad BC assignment");
	};
}

void CopyBlock(BC5_Block * dst, const uint8_t * src, int BC, int block_id)
{
	switch(BC)
	{
	case 4:
		dst->R = ((BC4_Block*)src)[block_id];
		break;
	case 5:
		*dst = ((BC5_Block*)src)[block_id];
		break;
	default:
		throw std::runtime_error("bad BC assignment");
	};
}
