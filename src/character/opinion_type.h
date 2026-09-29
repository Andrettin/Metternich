#pragma once

namespace metternich {

enum class opinion_type {
	hostile,
	unfavorable,
	cautious,
	indifferent,
	friendly,
	loyal
};

inline std::string_view get_opinion_type_name(const opinion_type opinion_type)
{
	switch (opinion_type) {
		case opinion_type::hostile:
			return "Hostile";
		case opinion_type::unfavorable:
			return "Unfavorable";
		case opinion_type::cautious:
			return "Cautious";
		case opinion_type::indifferent:
			return "Indifferent";
		case opinion_type::friendly:
			return "Friendly";
		case opinion_type::loyal:
			return "Loyal";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid opinion type: {}", std::to_underlying(opinion_type)));
}

}

Q_DECLARE_METATYPE(metternich::opinion_type)
