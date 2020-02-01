#ifndef GENERATEOCCLUSION_H
#define GENERATEOCCLUSION_H


class PngFile;
class DepthFile;

void GenerateOcclusion(PngFile & out, DepthFile & depth, PngFile & normals, int scan_distance);

#endif // GENERATEOCCLUSION_H
