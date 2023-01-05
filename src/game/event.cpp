#include "metternich.h"

#include "game/event.h"

#include "engine_interface.h"
#include "game/event_instance.h"
#include "game/game.h"
#include "script/condition/and_condition.h"

namespace metternich {

event::event(const std::string &identifier) : named_data_entry(identifier)
{
}

event::~event()
{
}

void event::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
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
