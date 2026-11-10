#include "metternich.h"

#include "game/event_random_group.h"

#include "database/defines.h"
#include "game/event_trigger.h"
#include "util/map_util.h"

namespace metternich {

const std::vector<event_random_group *> &event_random_group::get_all_of_trigger(const event_trigger trigger)
{
	static const std::vector<event_random_group *> empty_vector;
	return archimedes::map::get_or_default(event_random_group::groups_by_trigger, trigger, empty_vector);
}

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
			this->domain_events.push_back(nullptr);
			this->province_events.push_back(nullptr);
			this->site_events.push_back(nullptr);
		}
	}

	named_data_entry::initialize();
}

void event_random_group::check() const
{
}

int event_random_group::get_delay(const int current_year) const
{
	if (this->delay > 0) {
		return this->delay;
	}

	if (this->delay_days > 0) {
		return defines::get()->days_to_turns(this->delay_days, current_year).to_int();
	}

	return 0;
}

}

#include "moc_event_random_group.cpp"
