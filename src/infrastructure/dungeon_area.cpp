#include "metternich.h"

#include "infrastructure/dungeon_area.h"

#include "game/domain_event.h"
#include "script/condition/and_condition.h"

namespace metternich {

dungeon_area::dungeon_area(const std::string &identifier) : named_data_entry(identifier)
{
}

dungeon_area::~dungeon_area()
{
}

void dungeon_area::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void dungeon_area::initialize()
{
	//create event for dungeon area
	domain_event *event = domain_event::add(this->get_identifier(), this->get_module());
	event->set_name(this->get_name());
	event->set_portrait(this->get_portrait());
	event->set_description(this->get_description());

	this->event = event;

	named_data_entry::initialize();
}

void dungeon_area::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

}
