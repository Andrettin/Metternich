#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/assert_util.h"
#include "util/fractional_int.h"

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

	province_game_data *province_game_data = this->get_province()->get_game_data();

	if (this->get_building() != nullptr) {
		province_game_data->on_building_gained(this->get_building(), -1);
	}

	this->building = building;

	if (this->get_building() != nullptr) {
		province_game_data->on_building_gained(this->get_building(), 1);
	}

	if (game::get()->is_running()) {
		province_game_data->reassign_workers();

		emit building_changed();
	}
}

int building_slot::get_employment_capacity() const
{
	if (this->get_building() != nullptr) {
		return this->get_building()->get_employment_capacity();
	}

	return 0;
}

centesimal_int building_slot::get_output_multiplier() const
{
	if (this->get_building() != nullptr && this->get_building()->get_output_commodity() != nullptr) {
		centesimal_int output_multiplier = this->get_building()->get_output_multiplier();
		output_multiplier += centesimal_int(this->get_province()->get_game_data()->get_owner()->get_game_data()->get_commodity_production_modifier(this->get_building()->get_output_commodity())) / 100;
		return output_multiplier;
	}

	return centesimal_int(0);
}

}
