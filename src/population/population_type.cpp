#include "metternich.h"

#include "population/population_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "population/population_class.h"
#include "util/assert_util.h"

namespace metternich {

void population_type::initialize()
{
	assert_throw(this->population_class != nullptr);

	this->population_class->add_population_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_population_class_type(this->get_population_class()) == nullptr);

		this->culture->set_population_class_type(this->get_population_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_population_class_type(this->get_population_class()) == nullptr);

		this->cultural_group->set_population_class_type(this->get_population_class(), this);
	} else {
		this->population_class->set_default_population_type(this);
	}

	data_entry::initialize();
}

void population_type::check() const
{
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_small_icon() != nullptr);
}

}
