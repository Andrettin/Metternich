#pragma once

#include "util/enum_converter.h"

namespace metternich {

enum class event_trigger {
	none,
	quarterly_pulse,
	yearly_pulse,
	ruler_death,
	ruins_explored
};

}

extern template class archimedes::enum_converter<metternich::event_trigger>;

Q_DECLARE_METATYPE(metternich::event_trigger)
