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

		if (this->get_country() != nullptr) {
			country_game_data *country_game_data = this->get_country()->get_game_data();
			country_building_slot *country_building_slot = country_game_data->get_building_slot(this->get_building()->get_building_class()->get_slot_type());

			assert_throw(country_building_slot != nullptr);

			if (country_building_slot->can_have_building(this->get_building())) {
				country_building_slot->set_building(this->get_building());
			}
		}
	}
}

bool provincial_building_slot::can_have_building(const building_type *building) const
{
	if (!building->is_provincial()) {
		return false;
	}

	return building_slot::can_have_building(building);
}

const country *provincial_building_slot::get_country() const
{
	return this->get_province()->get_game_data()->get_owner();
}

}
