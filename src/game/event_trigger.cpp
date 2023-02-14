#include "metternich.h"

#include "game/event_trigger.h"

namespace archimedes {

template class enum_converter<metternich::event_trigger>;

template <>
const std::string enum_converter<metternich::event_trigger>::property_class_identifier = "metternich::event_trigger";

template <>
const std::map<std::string, metternich::event_trigger> enum_converter<metternich::event_trigger>::string_to_enum_map = {
	{ "none", metternich::event_trigger::none },
	{ "quarterly_pulse", metternich::event_trigger::quarterly_pulse },
	{ "yearly_pulse", metternich::event_trigger::yearly_pulse },
	{ "ruler_death", metternich::event_trigger::ruler_death },
	{ "ruins_explored", metternich::event_trigger::ruins_explored }
};

template <>
const bool enum_converter<metternich::event_trigger>::initialized = enum_converter::initialize();

}
