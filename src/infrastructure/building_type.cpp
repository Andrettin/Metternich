#include "metternich.h"

#include "infrastructure/building_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/employment_type.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

void building_type::initialize()
{
	assert_throw(this->building_class != nullptr);
	this->building_class->add_building_type(this);
	this->building_class->get_slot_type()->add_building_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_building_class_type(this->get_building_class()) == nullptr);

		this->culture->set_building_class_type(this->get_building_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_building_class_type(this->get_building_class()) == nullptr);

		this->cultural_group->set_building_class_type(this->get_building_class(), this);
	} else {
		this->building_class->set_default_building_type(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_building(this);
	}

	data_entry::initialize();
}

void building_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
	assert_throw(this->get_icon() != nullptr);

	if (this->get_employment_type() != nullptr) {
		assert_throw(this->get_output_commodity() != nullptr);
		assert_throw(this->get_output_multiplier() > 0);
		assert_throw(this->get_employment_capacity() > 0);
	}
}

const commodity *building_type::get_output_commodity() const
{
	if (this->get_employment_type() != nullptr) {
		return this->get_employment_type()->get_output_commodity();
	}

	return nullptr;
}

bool building_type::can_employ_worker(const population_unit *population_unit) const
{
	if (this->get_employment_type() == nullptr) {
		return false;
	}

	if (!vector::contains(this->get_employment_type()->get_employees(), population_unit->get_type()->get_population_class())) {
		return false;
	}

	return true;
}

}
