#pragma once

namespace metternich {

enum class spell_type {
	none,
	exploit,
	spell
};

inline std::string_view get_spell_type_name(const spell_type type)
{
	switch (type) {
		case spell_type::exploit:
			return "Exploit";
		case spell_type::spell:
			return "Spell";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid spell type: {}", std::to_underlying(type)));
}

//whether the spell type is learnable through items like scrolls
inline bool is_spell_type_item_learnable(const spell_type type)
{
	switch (type) {
		case spell_type::spell:
			return true;
		case spell_type::exploit:
			return false;
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid spell type: {}", std::to_underlying(type)));
}

//whether the spell type costs mana by default
inline bool does_spell_type_cost_mana(const spell_type type)
{
	switch (type) {
		case spell_type::spell:
			return true;
		case spell_type::exploit:
			return false;
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid spell type: {}", std::to_underlying(type)));
}

}

Q_DECLARE_METATYPE(metternich::spell_type)
