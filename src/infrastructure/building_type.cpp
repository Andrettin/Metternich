#include "metternich.h"

#include "infrastructure/building_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "infrastructure/building_class.h"
#include "util/assert_util.h"

namespace metternich {

void building_type::initialize()
{
	assert_throw(this->building_class != nullptr);
	this->building_class->add_building_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_building_class_type(this->get_building_class()) == nullptr);

		this->culture->set_building_class_type(this->get_building_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_building_class_type(this->get_building_class()) == nullptr);

		this->cultural_group->set_building_class_type(this->get_building_class(), this);
	} else {
		this->building_class->set_default_building_type(this);
	}

	data_entry::initialize();
}

void building_type::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
