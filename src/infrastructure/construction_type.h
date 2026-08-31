#pragma once

namespace metternich {

enum class construction_type {
	none,
	fortification,
	cultural_academy,
	engineering_academy,
	financial_academy,
	military_academy,
	naval_academy,
	religious_academy
};

inline std::string_view get_construction_type_name(const construction_type construction_type)
{
	switch (construction_type) {
		case construction_type::fortification:
			return "Fortification";
		case construction_type::cultural_academy:
			return "Cultural Academy";
		case construction_type::engineering_academy:
			return "Engineering Academy";
		case construction_type::financial_academy:
			return "Financial Academy";
		case construction_type::military_academy:
			return "Military Academy";
		case construction_type::naval_academy:
			return "Naval Academy";
		case construction_type::religious_academy:
			return "Religious Academy";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid construction type: \"{}\".", std::to_underlying(construction_type)));
}

}

Q_DECLARE_METATYPE(metternich::construction_type)
