#ifndef HEIGHT_MASK_H
#define HEIGHT_MASK_H
#include <glm/vec4.hpp>
#include <glm/gtc/type_precision.hpp>
#include <memory>

class PngFile;

std::unique_ptr<glm::ivec4[]> GetRowInfo(PngFile & in, uint32_t color);
std::unique_ptr<glm::u8vec2[]> GetRowMinMax(PngFile const& in);


#endif // HEIGHT_MASK_H
