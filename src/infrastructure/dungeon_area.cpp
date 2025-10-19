#include "metternich.h"

#include "infrastructure/dungeon_area.h"

#include "game/domain_event.h"
#include "script/condition/and_condition.h"

namespace metternich {

dungeon_area::dungeon_area(const std::string &identifier) : named_data_entry(identifier)
{
	//create event for dungeon area
	this->event = domain_event::add(this->get_identifier(), nullptr);
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
	} else if (tag == "immediate_effects" || tag == "option") {
		this->event->process_gsml_scope(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void dungeon_area::initialize()
{
	this->event->set_module(this->get_module());
	this->event->set_name(this->get_name());
	this->event->set_portrait(this->get_portrait());
	this->event->set_description(this->get_description());

	named_data_entry::initialize();
}

void dungeon_area::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

}
