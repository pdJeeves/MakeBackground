#include "blur_config.h"
#include "backgroundexception.h"
#include <nlohmann/json.hpp>
#include <fstream>

#if defined(FX_GLTF_HAS_CPP_17)
        template <typename TTarget>
        inline void ReadOptionalField(std::string_view key, nlohmann::json const & json, TTarget & target)
#else
        template <typename TKey, typename TTarget>
        inline void ReadOptionalField(TKey && key, nlohmann::json const & json, TTarget & target)
#endif
        {
            const nlohmann::json::const_iterator iter = json.find(key);
            if (iter != json.end())
            {
                target = iter->get<TTarget>();
            }
        }


void from_json(const nlohmann::json & json, BlurStage & db)
{
	ReadOptionalField("diff_power",  json, db.diff_power);
	ReadOptionalField("max_slope",   json, db.max_slope);
	ReadOptionalField("diff_factor", json, db.diff_factor);
	ReadOptionalField("diff_mul",    json, db.diff_mul);

	ReadOptionalField("min_delta",   json, db.min_delta);
	ReadOptionalField("max_delta",   json, db.max_delta);

	ReadOptionalField("cycles",      json, db.cycles);
	ReadOptionalField("clamp",       json, db.clamp);
	ReadOptionalField("commit",      json, db.commit);
	ReadOptionalField("color",       json, db.color);
}

std::vector<BlurStage> ReadConfiguration(std::string const& documentFilePath)
{
	try
	{
		nlohmann::json json;

		{
			std::ifstream file(documentFilePath);
			if (!file.is_open())
			{
				throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory));
			}

			file >> json;
		}

		std::vector<BlurStage> r = json;
		return r;
	}
	catch (std::system_error & e)
	{
		FileException("Couldn't open: " + documentFilePath + "\n" + e.what());
	}

	return std::vector<BlurStage>();
}
