#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class diplomacy_state {
	peace,
	alliance,
	war,
	non_aggression_pact,
	vassal,
	overlord,
	personal_union_subject,
	personal_union_overlord,
	colony,
	colonial_overlord
};

inline diplomacy_state get_diplomacy_state_counterpart(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::vassal:
			return diplomacy_state::overlord;
		case diplomacy_state::overlord:
			return diplomacy_state::vassal;
		default:
			return state;
	}

	throw std::runtime_error("Invalid diplomacy state: \"" + std::to_string(static_cast<int>(state)) + "\".");
}

inline std::string get_diplomacy_state_name(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::peace:
			return "Peace";
		case diplomacy_state::alliance:
			return "Alliance";
		case diplomacy_state::war:
			return "War";
		case diplomacy_state::non_aggression_pact:
			return "Non-Aggression Pact";
		case diplomacy_state::vassal:
			return "Vassal";
		case diplomacy_state::overlord:
			return "Overlord";
		default:
			break;
	}

	throw std::runtime_error("Invalid diplomacy state: \"" + std::to_string(static_cast<int>(state)) + "\".");
}

inline bool is_overlordship_diplomacy_state(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::overlord:
			return true;
		default:
			return false;
	}
}

inline bool is_vassalage_diplomacy_state(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::vassal:
			return true;
		default:
			return false;
	}
}

}

extern template class archimedes::enum_converter<metternich::diplomacy_state>;

Q_DECLARE_METATYPE(metternich::diplomacy_state)
