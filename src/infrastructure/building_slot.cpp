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

bool building_slot::can_have_building(const building_type *building) const
{
	if (building->get_required_building() != nullptr && this->get_building() != building->get_required_building()) {
		return false;
	}

	if (this->get_building() != nullptr) {
		if (building == this->get_building()) {
			return false;
		}

		if (building->get_employment_capacity() < this->get_building()->get_employment_capacity()) {
			return false;
		}

		if (building->get_output_multiplier() < this->get_building()->get_output_multiplier()) {
			return false;
		}

		if (building->get_employment_capacity() == this->get_building()->get_employment_capacity() && building->get_output_multiplier() == this->get_building()->get_output_multiplier()) {
			//the building must be better in some way
			return false;
		}
	}

	return true;
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
	if (this->get_building() == nullptr) {
		return centesimal_int(0);
	}

	const commodity *output_commodity = this->get_building()->get_output_commodity();

	if (output_commodity == nullptr) {
		return centesimal_int(0);
	}

	centesimal_int output_multiplier = this->get_building()->get_output_multiplier();

	const province_game_data *province_game_data = this->get_province()->get_game_data();
	int production_modifier = province_game_data->get_production_modifier() + province_game_data->get_commodity_production_modifier(output_commodity);

	if (province_game_data->get_owner() != nullptr) {
		const country_game_data *owner_game_data = province_game_data->get_owner()->get_game_data();
		production_modifier += owner_game_data->get_production_modifier() + owner_game_data->get_commodity_production_modifier(output_commodity);
	}

	output_multiplier += centesimal_int(production_modifier) / 100;

	return output_multiplier;
}

}
