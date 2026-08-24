#pragma once

namespace metternich {

enum class province_loyalty_level {
	rebellious = 0,
	hostile = 1,
	unfavorable = 2,
	aligned = 3,
	cooperative = 4,
	loyal = 5
};

inline std::string_view get_province_loyalty_level_name(const province_loyalty_level province_loyalty_level)
{
	switch (province_loyalty_level) {
		case province_loyalty_level::rebellious:
			return "Rebellious";
		case province_loyalty_level::hostile:
			return "Hostile";
		case province_loyalty_level::unfavorable:
			return "Unfavorable";
		case province_loyalty_level::aligned:
			return "Aligned";
		case province_loyalty_level::cooperative:
			return "Cooperative";
		case province_loyalty_level::loyal:
			return "Loyal";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid province loyalty level: {}", std::to_underlying(province_loyalty_level)));
}

}

Q_DECLARE_METATYPE(metternich::province_loyalty_level)
