#ifndef BLUR_CONFIG_H
#define BLUR_CONFIG_H
#include <vector>
#include <string>

struct BlurStage
{
	float diff_power{};
	float max_slope{};
	float diff_factor{};
	float diff_mul{};

	float min_delta{};
	float max_delta{};

	short cycles{};
	bool  clamp{};
	bool  commit{};
	int   color{-1};
};

std::vector<BlurStage> ReadConfiguration(std::string const& path);

#endif // BLUR_CONFIG_H
