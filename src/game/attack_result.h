#pragma once

namespace metternich {

enum class attack_result {
	none,
	miss,
	fall_back,
	hit,
	route,
	destroy
};

inline std::string_view get_attack_result_name(const attack_result result)
{
	switch (result) {
		case attack_result::miss:
		case attack_result::fall_back:
			return "Miss";
		case attack_result::hit:
		case attack_result::route:
			return "Hit";
		case attack_result::destroy:
			return "Destroy";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid attack result: {}", static_cast<int>(result)));
}

}

Q_DECLARE_METATYPE(metternich::attack_result)
