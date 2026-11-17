#include "metternich.h"

#include "game/event_random_group.h"

#include "database/defines.h"
#include "game/event_trigger.h"
#include "util/assert_util.h"
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

	assert_throw(this->get_none_weight() >= 0);
	const size_t none_weight = static_cast<size_t>(this->get_none_weight());
	if (none_weight > 0) {
		//add one null value per none weight to each scope type event vector
		std::apply([none_weight](auto &... event_vectors) {
			(event_vectors.insert(event_vectors.end(), none_weight, nullptr), ...);
		}, this->events);
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
