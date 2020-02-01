#ifndef BG_TYPE_H
#define BG_TYPE_H

enum class bg_Type : int
{
	Diffuse,
	Depth,
	Normals,
	Roughness,
	Occlusion
};

const char * bg_TypeName(bg_Type);

#endif // BG_TYPE_H
