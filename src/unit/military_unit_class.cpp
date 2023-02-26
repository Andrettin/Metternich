#include "metternich.h"

#include "unit/military_unit_class.h"

#include "unit/military_unit_category.h"
#include "unit/military_unit_domain.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

military_unit_class::military_unit_class(const std::string &identifier)
	: named_data_entry(identifier), domain(military_unit_domain::none), category(military_unit_category::none)
{
}

void military_unit_class::check() const
{
	assert_throw(this->get_domain() != military_unit_domain::none);
	assert_throw(this->get_category() != military_unit_category::none);
}

bool military_unit_class::is_animal() const
{
	switch (this->get_category()) {
		case military_unit_category::beasts:
		case military_unit_category::colossal_beasts:
		case military_unit_category::flying_beasts:
		case military_unit_category::colossal_flying_beasts:
			return true;
		default:
			return false;
	}
}

void military_unit_class::set_default_unit_type(const military_unit_type *unit_type)
{
	assert_throw(this->get_default_unit_type() == nullptr);

	this->default_unit_type = unit_type;
}

}
