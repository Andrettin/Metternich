#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "economy/production_type.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
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
		this->calculate_base_commodity_outputs();

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

		if (building->get_base_capacity() < this->get_building()->get_base_capacity()) {
			return false;
		}

		if (building->get_base_capacity() == this->get_building()->get_base_capacity()) {
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

bool building_slot::is_available() const
{
	if (this->get_building() != nullptr) {
		return true;
	}

	for (const building_type *building : this->get_type()->get_building_types()) {
		if (building->get_required_building() != nullptr) {
			continue;
		}

		if (!this->get_province()->get_game_data()->is_capital()) {
			const production_type *production_type = building->get_production_type();
			if (production_type != nullptr) {
				for (const auto &[input_commodity, input_multiplier] : production_type->get_input_commodities()) {
					if (!this->get_province()->get_game_data()->can_produce_commodity(input_commodity)) {
						return false;
					}
				}
			}
		}

		return true;
	}

	return false;
}

int building_slot::get_capacity() const
{
	if (this->get_building() != nullptr) {
		return this->get_building()->get_base_capacity();
	}

	return 0;
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

		const int input_value_int = std::abs(output_value_int);

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

void building_slot::calculate_base_commodity_outputs()
{
	this->base_commodity_outputs.clear();

	const building_type *building_type = this->get_building();
	if (building_type == nullptr) {
		return;
	}

	const production_type *production_type = building_type->get_production_type();
	if (production_type == nullptr) {
		return;
	}

	const commodity *output_commodity = production_type->get_output_commodity();
	centesimal_int output;

	/*
	for (const population_unit *employee : this->get_employees()) {
		output += employee->get_employment_output(production_type);
	}
	*/

	commodity_map<centesimal_int> inputs;

	for (const auto &[input_commodity, input_multiplier] : production_type->get_input_commodities()) {
		inputs[input_commodity] = input_multiplier * output / production_type->get_output_value();
	}

	this->base_commodity_outputs[output_commodity] += output;

	for (const auto &[input_commodity, input_value] : inputs) {
		this->base_commodity_outputs[input_commodity] -= input_value;
	}
}

void building_slot::apply_country_modifier(const country *country, const int multiplier)
{
	if (this->get_building() != nullptr && this->get_building()->get_country_modifier() != nullptr) {
		this->get_building()->get_country_modifier()->apply(country, multiplier);
	}
}

}
