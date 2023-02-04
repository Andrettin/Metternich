#include "metternich.h"

#include "game/event_random_group.h"

#include "database/defines.h"
#include "game/event.h"
#include "game/event_trigger.h"
#include "util/assert_util.h"

namespace metternich {

event_random_group::event_random_group(const std::string &identifier)
	: named_data_entry(identifier), trigger(event_trigger::none)
{
}

void event_random_group::initialize()
{
	event_random_group::groups_by_trigger[this->get_trigger()].push_back(this);

	if (this->get_none_weight() > 0) {
		for (int i = 0; i < this->get_none_weight(); ++i) {
			this->character_events.push_back(nullptr);
			this->country_events.push_back(nullptr);
		}
	}

	data_entry::initialize();
}

void event_random_group::check() const
{
	assert_throw(this->get_trigger() != event_trigger::none);
}

}
