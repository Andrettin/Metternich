#include "metternich.h"

#include "infrastructure/wonder.h"

#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "technology/technology.h"

namespace metternich {

wonder::wonder(const std::string &identifier) : named_data_entry(identifier)
{
}

wonder::~wonder()
{
}
	
void wonder::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "province_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		database::process_gsml_data(conditions, scope);
		this->province_conditions = std::move(conditions);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		database::process_gsml_data(this->province_modifier, scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void wonder::initialize()
{
	this->get_building()->get_building_class()->get_slot_type()->add_wonder(this);

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_wonder(this);
	}

	if (this->obsolescence_technology != nullptr) {
		this->obsolescence_technology->add_disabled_wonder(this);
	}

	named_data_entry::initialize();
}

void wonder::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Wonder \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_building() == nullptr) {
		throw std::runtime_error(std::format("Wonder \"{}\" has no building type.", this->get_identifier()));
	}

	if (!this->get_building()->is_provincial()) {
		throw std::runtime_error(std::format("Wonder \"{}\" has a non-provincial building type.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_province_conditions() != nullptr) {
		this->get_province_conditions()->check_validity();
	}
}

int wonder::get_score() const
{
	int score = 0;

	if (this->get_country_modifier() != nullptr) {
		score += this->get_country_modifier()->get_score();
	}

	if (this->get_province_modifier() != nullptr) {
		score += this->get_province_modifier()->get_score();
	}

	return score;
}

}
