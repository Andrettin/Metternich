#pragma once

namespace metternich {

enum class succession_gender_type {
	none,
	agnatic,
	agnatic_cognatic,
	cognatic,
	enatic,
	enatic_cognatic
};

inline std::string_view get_succession_gender_type_name(const succession_gender_type succession_gender_type)
{
	switch (succession_gender_type) {
		case succession_gender_type::agnatic:
			return "Agnatic";
		case succession_gender_type::agnatic_cognatic:
			return "Agnatic-Cognatic";
		case succession_gender_type::cognatic:
			return "Cognatic";
		case succession_gender_type::enatic:
			return "Enatic";
		case succession_gender_type::enatic_cognatic:
			return "Enatic-Cognatic";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid succession gender type: {}", static_cast<int>(succession_gender_type)));
}

}

Q_DECLARE_METATYPE(metternich::succession_gender_type)
