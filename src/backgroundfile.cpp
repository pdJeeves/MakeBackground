#define _CRT_SECURE_NO_WARNINGS
#include "backgroundfile.h"
#include "bg_type.h"
#include "ddsfile.h"
#include <cstring>
#include <algorithm>
#include <type_traits>
#include <climits>
#include <cassert>

const char * bg_TypeName(bg_Type it)
{
	const char * name[] =
	{
		"BaseColor",
		"Depth",
		"Normals",
		"Roughness",
		"Occlusion"
	};

	return name[(int)it];
}


bool isTransparent(DXT5_Block & m);
bool isTransparent(DXT1_Block & m);
bool isTransparent(BC4_Block & m);

BackgroundFile::BackgroundFile(const char * name) :
	FileBase(name)
{

}

void BackgroundFile::WriteOut()
{
	FILE * fp = fopen(path.c_str(), "wb");

	if(fp == nullptr)
	{
		perror("Failed to open output file: ");
		return;
	}
/*
	uint32_t version = 1;

	uint16_t width = tiles_x * 256;
	uint16_t height = tiles_y * 256;
	uint32_t image_offsets[32];
	memset(image_offsets, 0, 32*sizeof(uint32_t));

	image_offsets[0] = 4 * 34;

	fwrite(&version, sizeof(uint32_t), 1, fp);
	fwrite(&width , sizeof(uint16_t), 1, fp);
	fwrite(&height, sizeof(uint16_t), 1, fp);
	fwrite(image_offsets, sizeof(uint32_t), 32, fp);
*/
	WriteFile(fp);
	fclose(fp);
}

void BackgroundFile::WriteFile(FILE * fp)
{
	const char * title = "lbck";
	short version = 2;

	size_t offset = ftell(fp);
	uint16_t width = tiles_x * 256;
	uint16_t height = tiles_y * 256;

	fwrite(title, 1, 4, fp);
	fwrite(&version, 2, 1, fp);
	fwrite(&tiles_x, 1, 1, fp);
	fwrite(&tiles_y, 1, 1, fp);
	fwrite(&width , sizeof(uint16_t), 1, fp);
	fwrite(&height, sizeof(uint16_t), 1, fp);

	fwrite(&tile_info[0], sizeof(TileInfo), length(), fp);

	std::vector<uint32_t> mip(3 * length() + 1, 0);

	fpos_t position;
	fgetpos(fp, &position);
	fseek(fp, sizeof(uint32_t) * mip.size(), SEEK_CUR);

	for(int i = 0; i < length(); ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			mip[i*3 + j] = ftell(fp) - offset;

			if(encoded[j] != nullptr && encoded[j][i].size())
			{
				auto & vec = encoded[j][i];
				fwrite(&vec[0], sizeof(uint8_t), encoded[j][i].size(), fp);
			}
		}
	}

	mip.back() = ftell(fp) - offset;

	fsetpos(fp, &position);
	fwrite(&mip[0], sizeof(mip[0]), mip.size(), fp);
}

void * deinterleave_primitive(void * r_dst, void * r_src, int blocks, int bytes, int stride)
{
	uint8_t * dst = (uint8_t*) r_dst;
	uint8_t * src = (uint8_t*) r_src;

	for(int j = 0; j < blocks; ++j)
	{
		for(int i = 0; i < bytes; ++i)
		{
			*dst = src[j * stride + i];
			++dst;
		}
	}

	return dst;
}

void * deinterleave_primitive_reps(void * r_dst, void * r_src, int blocks, int bytes)
{
	uint8_t * dst = (uint8_t*) r_dst;
	uint8_t * src = (uint8_t*) r_src;

	for(int j = 0; j < blocks; ++j)
	{
		for(int i = 0; i < bytes; ++i)
		{
			*dst = src[i];
			++dst;
		}
	}

	return dst;
}


#define DEINTERLEAVE 1

void * deinterleave_block(void * dst, DXT1_Block * src, int blocks)
{
#if DEINTERLEAVE
	dst = deinterleave_primitive_reps(dst, &src[0].c[0], blocks, 2);
	dst = deinterleave_primitive_reps(dst, &src[0].c[1], blocks, 2);
	dst = deinterleave_primitive_reps(dst, &src[0].i[0], blocks, 4);
	struct DXT1_Block
	{
		uint16_t c[2];
		uint8_t  i[4];
	};

	return dst;

#else
	auto * i   = (DXT1_Block*) dst;
	auto * end = i + blocks;

	for(; i < end; ++i)
		*i = *src;

	return end;
#endif
}

void * deinterleave_block(void * dst, BC4_Block * src, int blocks)
{
#if DEINTERLEAVE
	dst = deinterleave_primitive_reps(dst, &src[0].c[0], blocks, 1);
	dst = deinterleave_primitive_reps(dst, &src[0].c[1], blocks, 1);
	dst = deinterleave_primitive_reps(dst, &src[0].i[0], blocks, 6);

	return dst;
#else
	auto * i   = (BC4_Block*) dst;
	auto * end = i + blocks;

	for(; i < end; ++i)
		*i = *src;

	return end;
#endif
}

void * deinterleave_block(void * dst, DXT5_Block * src, int blocks)
{
	dst = deinterleave_block(dst, &src[0].alpha, blocks);
	dst = deinterleave_block(dst, &src[0].color, blocks);

	return dst;
}

void * deinterleave_block(void * dst, BC5_Block * src, int blocks)
{
	dst = deinterleave_block(dst, &src[0].R, blocks);
	dst = deinterleave_block(dst, &src[0].G, blocks);

	return dst;
}


void * deinterleave(void * dst, DXT1_Block * src, int blocks, int stride = sizeof(DXT1_Block))
{
#if DEINTERLEAVE
	dst = deinterleave_primitive(dst, &src[0].c[0], blocks, 2, stride);
	dst = deinterleave_primitive(dst, &src[0].c[1], blocks, 2, stride);
	dst = deinterleave_primitive(dst, &src[0].i[0], blocks, 4, stride);

	return dst;
#else
	memcpy(dst, &src[0], blocks * sizeof(src[0]));
	return (uint8_t *) dst + (blocks * sizeof(src[0]));
#endif
}


void * deinterleave(void * dst, BC4_Block * src, int blocks, int stride = sizeof(BC4_Block))
{
#if DEINTERLEAVE
	dst = deinterleave_primitive(dst, &src[0].c[0], blocks, 1, stride);
	dst = deinterleave_primitive(dst, &src[0].c[1], blocks, 1, stride);
	dst = deinterleave_primitive(dst, &src[0].i[0], blocks, 6, stride);

	return dst;
#else
	memcpy(dst, &src[0], blocks * sizeof(src[0]));
	return (uint8_t *) dst + (blocks * sizeof(src[0]));
#endif
}

void * deinterleave(void * dst, DXT5_Block * src, int blocks)
{
	dst = deinterleave(dst, &src[0].alpha, blocks, sizeof(DXT5_Block));
	dst = deinterleave(dst, &src[0].color, blocks, sizeof(DXT5_Block));

	return dst;
}

void * deinterleave(void * dst, BC5_Block * src, int blocks)
{
	dst = deinterleave(dst, &src[0].R, blocks, sizeof(BC5_Block));
	dst = deinterleave(dst, &src[0].G, blocks, sizeof(BC5_Block));

	return dst;
}

void BackgroundFile::Deinterleave()
{
	enum
	{
		DiffOffset        = 0,
		RoughOffset       = DiffOffset      + sizeof(DXT5_Block),
		DepthOffset       = RoughOffset     + sizeof(DXT5_Block),
		NormalOffset      = DepthOffset     + sizeof(BC5_Block),
		DeinterlacedBytes = NormalOffset    + sizeof(BC5_Block),
	};

	BC5_Block default_depth;
	BC5_Block default_normals;

	DXT5_Block default_roughness;
	DXT5_Block default_diffuse;

	memset(&default_depth,     0, sizeof(default_depth));
	memset(&default_normals,   0, sizeof(default_normals));
	memset(&default_roughness, 0, sizeof(default_roughness));
	memset(&default_diffuse,   0, sizeof(default_diffuse));

//default diffuse is solid white
	default_diffuse.color.c[0] = 0xFFFF;
	default_diffuse.alpha.c[0] = 255;

//default roughness is 50% rough dialectric
	default_roughness.color.c[0] = 0x0400;
	default_roughness.alpha.c[0] = 255;

//default depth is 1 away from sky
	default_depth.R.c[0] = 1;

//default normals point forward
	default_normals.R.c[0] = 128;
	default_normals.G.c[0] = 128;

	for(int i = 0; i < 3; ++i)
	{
		if(encoded[i] == nullptr)
			encoded[i] = std::unique_ptr<std::vector<uint8_t>[]>(new std::vector<uint8_t>[length()]);

		for(int j = 0; j < length(); ++j)
		{
			const int w = (tile_info[j].x1 - tile_info[j].x0) << (2 - i);
			const int h = (tile_info[j].y1 - tile_info[j].y0) << (2 - i);

			const size_t blocks = (w*h);
			if(blocks == 0) continue;

			const size_t bytes = blocks * DeinterlacedBytes;

			encoded[i][j].resize(bytes);
			auto & vec = encoded[i][j];

			if(base_color[i] && base_color[i][j])
			{
				deinterleave(&vec[blocks*DiffOffset], base_color[i][j].get(), blocks);
				base_color[i][j].reset();
			}
			else
			{
				deinterleave_block(&vec[blocks*DiffOffset], &default_diffuse, blocks);
			}

			if(roughness[i] && roughness[i][j])
			{
				deinterleave(&vec[blocks*RoughOffset], roughness[i][j].get(), blocks);
				roughness[i][j].reset();
			}
			else
			{
				deinterleave_block(&vec[blocks*RoughOffset], &default_roughness, blocks);
			}

			if(depth[i] && depth[i][j])
			{
				deinterleave(&vec[blocks*DepthOffset], depth[i][j].get(), blocks);
				depth[i][j].reset();
			}
			else
			{
				deinterleave_block(&vec[blocks*RoughOffset], &default_depth, blocks);
			}

			if(normal[i] && normal[i][j])
			{
				deinterleave(&vec[blocks*NormalOffset], normal[i][j].get(), blocks);
				normal[i][j].reset();
			}
			else
			{
				deinterleave_block(&vec[blocks*RoughOffset], &default_normals, blocks);
			}
		}

		base_color[i].reset();
		roughness[i].reset();
		depth[i].reset();
		normal[i].reset();
	}
}

struct Dimensions
{
	short min_x{64};
	short min_y{64};
	short max_x{0};
	short max_y{0};

	bool isFull()
	{
		return min_x == 0 && min_y == 0 && max_x == 64 && max_y == 64;
	}
};

template<typename T>
Dimensions GetDimensions(T * t, Dimensions d)
{
	if((!isTransparent(t[0]) && !isTransparent(t[64*64-1]))
	|| (!isTransparent(t[63]) && !isTransparent(t[64*(64-1)])))
	{
		d.min_x = 0;
		d.min_y = 0;
		d.max_x = 64;
		d.max_y = 64;
		return d;
	}

	for(short y = 0; y < 64; ++y)
	{
		short min_x = SHRT_MAX;
		short max_x = SHRT_MIN;

		for(short x = 0; x < 64; ++x)
		{
			if(!isTransparent(t[y*64 + x]))
			{
				min_x = x;
				max_x = x+1;
				break;
			}
		}

		for(short x = 63; x >= min_x; --x)
		{
			if(!isTransparent(t[y*64 + x]))
			{
				max_x = x+1;
				break;
			}
		}

		if(min_x <= max_x)
		{
			d.min_x = std::min(min_x, d.min_x);
			d.max_x = std::max(max_x, d.max_x);
			d.min_y = std::min(y    , d.min_y);
			d.max_y = y+1;
		}
	}

	return d;
}


int BackgroundFile::CreateTileDimensions()
{
	SetTileCount(base_color);
	SetTileCount(depth);
	SetTileCount(normal);
	SetTileCount(roughness);

	tile_info = std::unique_ptr<TileInfo[]>(new TileInfo[length()]);

	int set = 0;

	for(int i = 0; i < length(); ++i)
	{
		BC5_Block * dp = depth[0][i].get();

		Dimensions d;

		d = GetDimensions(dp, d);

		if(!d.isFull())
		{
			if(base_color[0]    != nullptr
			&& base_color[0][i] != nullptr)
			{
				d = GetDimensions(base_color[0][i].get(), d);
			}
		}

		TileInfo * info = &tile_info[i];

		if(d.max_x < d.min_x)
		{
			info->x0 = 0;
			info->y0 = 0;
			info->x1 = 0;
			info->y1 = 0;
		}
		else
		{
			info->x0 = d.min_x / 4;
			info->y0 = d.min_y / 4;
			info->x1 = (d.max_x + 3) / 4;
			info->y1 = (d.max_y + 3) / 4;
		}

		++set;
	}

	return set;
}

template<typename T>
std::unique_ptr<T[]> ApplyTileDimensions(BackgroundFile::TileInfo & dimensions, std::unique_ptr<T[]> & blocks, int mip)
{
	size_t offset = 4 - mip;

	if(typeid(T) != typeid(uint16_t))
		offset -= 2;

	const uint16_t x0 = dimensions.x0 << offset;
	const uint16_t x1 = dimensions.x1 << offset;
	const uint16_t y0 = dimensions.y0 << offset;
	const uint16_t y1 = dimensions.y1 << offset;

	const int16_t w  = x1 - x0;
	const int16_t h  = y1 - y0;

	std::unique_ptr<T[]> r(new T[w*h]);

	for(int16_t y = 0; y < h; ++y)
	{
		memcpy(&r[y*w], &blocks[(y0 + y)*(64 >> mip) + x0] , w*sizeof(T));
	}

	return r;
}

template<typename T>
void ApplyTileDimensionsTemplate(BackgroundFile * _this, std::unique_ptr<std::unique_ptr<T[]>[]> & it, int mip, int j)
{
	if(it == nullptr)
		return;

	if(_this->tile_info[j].x1 == 0)
		it[j].reset();
	else
		it[j] = ApplyTileDimensions(_this->tile_info[j], it[j], mip);
}


void BackgroundFile::ApplyTileDimensions()
{
	int max = CreateTileDimensions();

	if(max == 0)
		return;

	for(int j = 0; j < length(); ++j)
	{
		if(tile_info[j].x0 == 0 && tile_info[j].y0 == 0 && tile_info[j].x1 == 16 && tile_info[j].y1 == 16)
			continue;

		for(int i = 0; i < 3; ++i)
		{
			ApplyTileDimensionsTemplate(this, depth[i], i, j);
			ApplyTileDimensionsTemplate(this, normal[i], i, j);
			ApplyTileDimensionsTemplate(this, base_color[i], i, j);
			ApplyTileDimensionsTemplate(this, roughness[i], i, j);
		}
	}
}

bool isTransparent(BC5_Block & m)
{
	return isTransparent(m.R) && isTransparent(m.G);
}


bool isTransparent(BC4_Block & m)
{
	static const uint8_t zero_strings[3][6]
	{
		{0, 0, 0, 0, 0, 0},
		{0x24, 0x92, 0x49, 0x24, 0x92, 0x49},
		{0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6},
	};

	return (!m.c[0] && !m.c[1])
	    || (m.c[0] < m.c[1] && !memcmp(zero_strings[2], m.i, 6))
	    || (!m.c[0] && !memcmp(zero_strings[0], m.i, 6))
	    || (!m.c[1] && !memcmp(zero_strings[1], m.i, 6));
}

bool isTransparent(DXT1_Block & m)
{
	return m.c[0] < m.c[1]
		&& m.i[0] == 0xFF
		&& m.i[1] == 0xFF
		&& m.i[2] == 0xFF
		&& m.i[3] == 0xFF;
}

bool isTransparent(DXT5_Block & m)
{
	return isTransparent(m.alpha);
}
