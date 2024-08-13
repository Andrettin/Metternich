#include "metternich.h"

#include "unit/civilian_unit_class.h"

#include "unit/civilian_unit_type.h"
#include "util/assert_util.h"

namespace metternich {

void civilian_unit_class::set_default_unit_type(const civilian_unit_type *unit_type)
{
	if (this->get_default_unit_type() != nullptr) {
		throw std::runtime_error(std::format("Cannot set \"{}\" as the default civilian unit type of class \"{}\", as it already has \"{}\" as its default type.", unit_type->get_identifier(), this->get_identifier(), this->get_default_unit_type()->get_identifier()));
	}

	this->default_unit_type = unit_type;
}

}
