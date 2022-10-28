#include "metternich.h"

#include "unit/civilian_unit_class.h"

#include "util/assert_util.h"

namespace metternich {

void civilian_unit_class::set_default_unit_type(const civilian_unit_type *unit_type)
{
	assert_throw(this->get_default_unit_type() == nullptr);

	this->default_unit_type = unit_type;
}

}
