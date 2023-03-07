#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/employment_type.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population_unit.h"
#include "script/modifier.h"
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

		if (this->get_building()->get_province_modifier() != nullptr) {
			this->get_building()->get_province_modifier()->apply(this->get_province(), -1);
		}

		if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			this->get_building()->get_country_modifier()->apply(this->get_country(), -1);
		}
	}

	this->building = building;

	if (this->get_building() != nullptr) {
		province_game_data->on_building_gained(this->get_building(), 1);

		if (this->get_building()->get_province_modifier() != nullptr) {
			this->get_building()->get_province_modifier()->apply(this->get_province(), 1);
		}

		if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			this->get_building()->get_country_modifier()->apply(this->get_country(), 1);
		}
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

const country *building_slot::get_country() const
{
	return this->get_province()->get_game_data()->get_owner();
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

commodity_map<centesimal_int> building_slot::get_base_commodity_outputs() const
{
	commodity_map<centesimal_int> output_per_commodity;

	const building_type *building_type = this->get_building();
	if (building_type == nullptr) {
		return output_per_commodity;
	}

	if (building_type->get_employment_type() == nullptr) {
		return output_per_commodity;
	}

	const employment_type *employment_type = building_type->get_employment_type();
	const commodity *output_commodity = employment_type->get_output_commodity();
	centesimal_int output;

	for (const population_unit *employee : this->get_employees()) {
		output += employee->get_employment_output(employment_type);
	}

	const centesimal_int output_multiplier = this->get_output_multiplier();

	output *= output_multiplier;

	commodity_map<centesimal_int> inputs;

	for (const auto &[input_commodity, input_multiplier] : employment_type->get_input_commodities()) {
		inputs[input_commodity] = input_multiplier * output / employment_type->get_output_multiplier();
	}

	output_per_commodity[output_commodity] += output;

	for (const auto &[input_commodity, input_value] : inputs) {
		output_per_commodity[input_commodity] -= input_value;
	}

	return output_per_commodity;
}

commodity_map<centesimal_int> building_slot::get_commodity_outputs() const
{
	commodity_map<centesimal_int> output_per_commodity = this->get_base_commodity_outputs();

	int input_fulfilled_percent = 100;

	const country *owner = this->get_province()->get_game_data()->get_owner();

	//check if inputs are fulfilled, and to which proportion
	for (const auto &[commodity, output_value] : output_per_commodity) {
		const int output_value_int = output_value.to_int();

		if (output_value_int >= 0) {
			//must be an input
			continue;
		}

		const int input_value_int = output_value_int * -1;

		const int available_input = owner ? owner->get_game_data()->get_stored_commodity(commodity) : 0;

		if (input_value_int < available_input) {
			input_fulfilled_percent = std::min(input_fulfilled_percent, available_input * 100 / input_value_int);
		}
	}

	if (input_fulfilled_percent < 100) {
		for (auto &[commodity, output_value] : output_per_commodity) {
			output_value *= input_fulfilled_percent;
			output_value /= 100;
		}
	}

	return output_per_commodity;
}

void building_slot::apply_country_modifier(const country *country, const int multiplier)
{
	if (this->get_building() != nullptr && this->get_building()->get_country_modifier() != nullptr) {
		this->get_building()->get_country_modifier()->apply(country, multiplier);
	}
}

}
