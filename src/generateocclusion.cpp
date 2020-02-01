#include "generateocclusion.h"
#include "depthfile.h"
#include "png_file.h"
#include <glm/vec2.hpp>

/* create memo of
	current angle
	arc distance
	sine
*/
void GenerateOcclusion(PngFile & out, DepthFile & depth, PngFile & normals, int)
{
}

/*
glm::vec3 GetNormal(PngFile const& normals, int x, int y)
{
	glm::vec3 r(
		normals.row_pointers[y][x*normals.channels + 0],
		normals.row_pointers[y][x*normals.channels + 1],
		normals.row_pointers[y][x*normals.channels + 2]);

	return glm::normalize(r * 2.f - 1.f);
}

void GenerateOcclusion(PngFile & out, DepthFile & depth, PngFile & normals, int s_r)
{
	normals.Read();

	int width = depth.size.x;
	int height = depth.size.y;
	int s_d    = s_r * 2 + 1;

//heights of surrounding parts...
	std::unique_ptr<glm::vec2[]> memo(new glm::vec2[s_d*s_d]);

	int prev_platform = -1;

	for(int y = 0; y < height; ++y)
	{
//copy middle in
		int y_begin = std::max(0,          y - s_r);
		int y_end   = std::min(height - 1, y + s_r);



//copy book ends
		int N = s_r - (y_begin - y);
		for(int y = 0; y < N; ++y)
			memcpy(&memo[y*s_d], &memo[N*s_d], s_d * sizeof(glm::vec2));

		N = (s_r) + (y_end - y);
		for(int y = N + 1; y < s_d; ++y)
			memcpy(&memo[y*s_d], &memo[N*s_d], s_d * sizeof(glm::vec2));

		for(int x = 0; x = width; ++x)
		{


		}
	}

}
*/
