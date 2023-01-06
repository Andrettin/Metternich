#include "metternich.h"

#include "game/event.h"

#include "engine_interface.h"
#include "game/event_instance.h"
#include "game/event_option.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "util/assert_util.h"

namespace metternich {

event::event(const std::string &identifier) : named_data_entry(identifier), trigger(event_trigger::none)
{
}

event::~event()
{
}

void event::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator gsml_operator = property.get_operator();
	const std::string &value = property.get_value();

	if (key == "random_weight") {
		if (gsml_operator != gsml_operator::assignment) {
			throw std::runtime_error("Invalid operator for property \"" + key + "\".");
		}

		this->set_random_weight(std::stoi(value));
	} else {
		data_entry::process_gsml_property(property);
	}
}

void event::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "random_weight_factor") {
		this->random_weight_factor = std::make_unique<factor<country>>();
		database::process_gsml_data(this->random_weight_factor, scope);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "option") {
		auto option = std::make_unique<event_option>();
		database::process_gsml_data(option, scope);
		this->options.push_back(std::move(option));
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void event::initialize()
{
	if (this->get_trigger() != event_trigger::none) {
		if (this->is_random()) {
			event::trigger_random_events[this->get_trigger()].push_back(this);
		} else {
			event::trigger_events[this->get_trigger()].push_back(this);
		}
	}

	data_entry::initialize();
}

void event::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	for (const std::unique_ptr<event_option> &option : this->get_options()) {
		option->check();
	}
}

void event::set_random_weight(const int weight)
{
	if (weight != 0) {
		this->random_weight_factor = std::make_unique<factor<country>>(weight);
	} else {
		this->random_weight_factor.reset();
	}
}

void event::fire(const country *country) const
{
	if (country != game::get()->get_player_country()) {
		//the event doesn't need to be displayed for AIs; instead, it should be processed immediately
		return;
	}

	auto event_instance = make_qunique<metternich::event_instance>(this, this->get_name_qstring(), this->get_description_qstring());
	engine_interface::get()->add_event_instance(std::move(event_instance));
}

}
