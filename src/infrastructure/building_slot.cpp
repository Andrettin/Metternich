#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "util/assert_util.h"

namespace metternich {

building_slot::building_slot(const building_slot_type *type, const metternich::province *province)
	: type(type), province(province)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_province() != nullptr);
}

void building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	if (building != nullptr) {
		assert_throw(building->get_building_class()->get_slot_type() == this->get_type());
	}

	this->building = building;

	emit building_changed();
}

}
