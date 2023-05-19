#include "metternich.h"

#include "infrastructure/provincial_building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "infrastructure/country_building_slot.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"

namespace metternich {

provincial_building_slot::provincial_building_slot(const building_slot_type *type, const metternich::province *province)
	: building_slot(type), province(province)
{
	assert_throw(this->get_province() != nullptr);
}


void provincial_building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	province_game_data *province_game_data = this->get_province()->get_game_data();

	const building_type *old_building = this->get_building();

	if (old_building != nullptr) {
		province_game_data->on_building_gained(old_building, -1);
	}

	building_slot::set_building(building);

	if (this->get_building() != nullptr) {
		province_game_data->on_building_gained(this->get_building(), 1);
	}
}

bool provincial_building_slot::can_have_building(const building_type *building) const
{
	if (!building->is_provincial()) {
		return false;
	}

	if (building->get_province_conditions() != nullptr) {
		if (!building->get_province_conditions()->check(this->get_province(), read_only_context(this->get_province()))) {
			return false;
		}
	}

	return building_slot::can_have_building(building);
}

const country *provincial_building_slot::get_country() const
{
	return this->get_province()->get_game_data()->get_owner();
}

}
