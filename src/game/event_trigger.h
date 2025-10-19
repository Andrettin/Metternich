#pragma once

namespace metternich {

enum class event_trigger {
	none,
	per_turn_pulse,
	quarterly_pulse,
	yearly_pulse,
	ruler_death,
	site_visited,
	dungeon_cleared,
	combat_started
};

inline std::string_view get_event_trigger_name(const event_trigger event_trigger)
{
	switch (event_trigger) {
		case event_trigger::per_turn_pulse:
			return "Per Turn Pulse";
		case event_trigger::quarterly_pulse:
			return "Quarterly Pulse";
		case event_trigger::yearly_pulse:
			return "Yearly Pulse";
		case event_trigger::ruler_death:
			return "Ruler Death";
		case event_trigger::site_visited:
			return "Site Visited";
		case event_trigger::dungeon_cleared:
			return "Dungeon Cleared";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid event trigger: \"{}\".", static_cast<int>(event_trigger)));
}

}

Q_DECLARE_METATYPE(metternich::event_trigger)
