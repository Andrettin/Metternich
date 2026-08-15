#pragma once

namespace metternich {

enum class spell_target {
	none,
	enemy,
	enemy_character,
	ally,
	ally_character
};

inline std::string_view get_spell_target_name(const spell_target target)
{
	switch (target) {
		case spell_target::enemy:
			return "Enemy";
		case spell_target::enemy_character:
			return "Enemy Character";
		case spell_target::ally:
			return "Ally";
		case spell_target::ally_character:
			return "Ally Character";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid spell target: {}", std::to_underlying(target)));
}

}

Q_DECLARE_METATYPE(metternich::spell_target)
