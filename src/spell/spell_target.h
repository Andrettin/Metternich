#pragma once

namespace metternich {

enum class spell_target {
	none,
	enemy,
	ally
};

inline std::string_view get_spell_target_name(const spell_target target)
{
	switch (target) {
		case spell_target::enemy:
			return "Enemy";
		case spell_target::ally:
			return "Ally";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid spell target: {}", static_cast<int>(target)));
}

}

Q_DECLARE_METATYPE(metternich::spell_target)
