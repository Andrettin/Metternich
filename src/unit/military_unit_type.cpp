#include "metternich.h"

#include "unit/military_unit_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "technology/technology.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_domain.h"
#include "util/assert_util.h"

namespace metternich {

void military_unit_type::initialize()
{
	assert_throw(this->unit_class != nullptr);

	this->unit_class->add_unit_type(this);

	if (!this->get_unit_class()->is_animal()) {
		if (this->culture != nullptr) {
			this->culture->set_military_class_unit_type(this->get_unit_class(), this);
		} else if (this->cultural_group != nullptr) {
			assert_throw(this->cultural_group->get_military_class_unit_type(this->get_unit_class()) == nullptr);

			this->cultural_group->set_military_class_unit_type(this->get_unit_class(), this);
		} else {
			this->unit_class->set_default_unit_type(this);
		}
	}

	if (this->required_technology != nullptr) {
		assert_throw(this->get_unit_class() != nullptr);
		this->required_technology->add_enabled_military_unit(this);
	}

	data_entry::initialize();
}

void military_unit_type::check() const
{
	assert_throw(this->get_domain() != military_unit_domain::none);
	assert_throw(this->get_icon() != nullptr);
}

military_unit_category military_unit_type::get_category() const
{
	if (this->get_unit_class() == nullptr) {
		return military_unit_category::none;
	}

	return this->get_unit_class()->get_category();
}

military_unit_domain military_unit_type::get_domain() const
{
	if (this->get_unit_class() == nullptr) {
		return military_unit_domain::none;
	}

	return this->get_unit_class()->get_domain();
}

}
