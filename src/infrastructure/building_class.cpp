#include "metternich.h"

#include "infrastructure/building_class.h"

#include "infrastructure/building_type.h"
#include "util/assert_util.h"

namespace metternich {

void building_class::set_default_building_type(const building_type *building_type)
{
	assert_throw(this->get_default_building_type() == nullptr);

	this->default_building_type = building_type;
}

}
