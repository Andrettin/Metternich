#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "population/population_unit.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/fractional_int.h"
#include "util/vector_util.h"

namespace metternich {

building_slot::building_slot(const building_slot_type *type, const metternich::country *country)
	: type(type), country(country)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_country() != nullptr);
}

void building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	if (building != nullptr) {
		assert_throw(building->get_building_class()->get_slot_type() == this->get_type());
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();

	const building_type *old_building = this->get_building();

	if (old_building != nullptr) {
		country_game_data->on_building_gained(old_building, -1);

		if (old_building->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			old_building->get_country_modifier()->apply(this->get_country(), -1);
		}
	}

	this->building = building;

	if (this->get_building() != nullptr) {
		country_game_data->on_building_gained(this->get_building(), 1);

		if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			this->get_building()->get_country_modifier()->apply(this->get_country(), 1);
		}
	}

	//clear production for production types which are no longer valid
	if (old_building != nullptr && !old_building->get_production_types().empty()) {
		if (building == nullptr || building->get_production_types() != old_building->get_production_types()) {
			for (const production_type *production_type : old_building->get_production_types()) {
				if (building != nullptr && vector::contains(building->get_production_types(), production_type)) {
					continue;
				}

				while (this->get_production_type_employed_capacity(production_type) > 0) {
					this->decrease_production(production_type);
				}
			}
		}
	}

	if (game::get()->is_running()) {
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

int building_slot::get_capacity() const
{
	if (this->get_building() != nullptr) {
		return this->get_building()->get_base_capacity();
	}

	return 0;
}

bool building_slot::can_increase_production(const production_type *production_type) const
{
	assert_throw(this->get_building() != nullptr);
	assert_throw(vector::contains(this->get_building()->get_production_types(), production_type));

	if (this->get_employed_capacity() >= this->get_capacity()) {
		return false;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		if (input_commodity->is_storable()) {
			if (country_game_data->get_stored_commodity(input_commodity) < input_value) {
				return false;
			}
		} else {
			//for non-storable commodities, like Labor, the commodity output is used directly instead of storage
			if (country_game_data->get_net_commodity_output(input_commodity) < input_value) {
				return false;
			}
		}
	}

	return true;
}

void building_slot::increase_production(const production_type *production_type)
{
	assert_throw(this->can_increase_production(production_type));

	++this->employed_capacity;
	++this->production_type_employed_capacities[production_type];

	country_game_data *country_game_data = this->get_country()->get_game_data();

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		if (input_commodity->is_storable()) {
			country_game_data->change_stored_commodity(input_commodity, -input_value);
		}
		country_game_data->change_commodity_input(input_commodity, input_value);
	}

	country_game_data->change_commodity_output(production_type->get_output_commodity(), production_type->get_output_value());
}

bool building_slot::can_decrease_production(const production_type *production_type) const
{
	assert_throw(this->get_building() != nullptr);
	assert_throw(vector::contains(this->get_building()->get_production_types(), production_type));

	if (this->get_employed_capacity() == 0) {
		return false;
	}

	if (this->get_production_type_employed_capacity(production_type) == 0) {
		return false;
	}

	return true;
}

void building_slot::decrease_production(const production_type *production_type)
{
	assert_throw(this->can_decrease_production(production_type));

	--this->employed_capacity;

	const int remaining_production_type_employed_capacity = --this->production_type_employed_capacities[production_type];
	assert_throw(remaining_production_type_employed_capacity >= 0);

	if (remaining_production_type_employed_capacity == 0) {
		this->production_type_employed_capacities.erase(production_type);
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		if (input_commodity->is_storable()) {
			country_game_data->change_stored_commodity(input_commodity, input_value);
		}
		country_game_data->change_commodity_input(input_commodity, -input_value);
	}

	country_game_data->change_commodity_output(production_type->get_output_commodity(), -production_type->get_output_value());
}

void building_slot::apply_country_modifier(const metternich::country *country, const int multiplier)
{
	if (this->get_building() != nullptr && this->get_building()->get_country_modifier() != nullptr) {
		this->get_building()->get_country_modifier()->apply(country, multiplier);
	}
}

}
