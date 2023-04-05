#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/container_util.h"
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
					this->decrease_production(production_type, true);
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

		if (building->get_score() < this->get_building()->get_score()) {
			return false;
		}

		if (building->get_base_capacity() == this->get_building()->get_base_capacity() && building->get_score() == this->get_building()->get_score()) {
			//the building must be better in some way
			return false;
		}
	}

	return true;
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

		if (building->get_conditions() != nullptr && !building->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			continue;
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

std::vector<const production_type *> building_slot::get_available_production_types() const
{
	if (this->get_building() == nullptr) {
		return {};
	}

	std::vector<const production_type *> production_types;

	for (const production_type *production_type : this->get_building()->get_production_types()) {
		if (production_type->get_output_commodity() == defines::get()->get_advisor_commodity() && !game::get()->get_rules()->are_advisors_enabled()) {
			continue;
		}

		production_types.push_back(production_type);
	}

	return production_types;
}

QVariantList building_slot::get_available_production_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_production_types());
}

int building_slot::get_production_type_output(const production_type *production_type) const
{
	int output = this->get_production_type_employed_capacity(production_type);

	if (output == 0) {
		return 0;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();
	const int output_modifier = country_game_data->get_output_modifier() + country_game_data->get_commodity_output_modifier(production_type->get_output_commodity());
	if (output_modifier != 0) {
		output *= 100 + output_modifier;
		output /= 100;

		output = std::max(output, 0);
	}

	return output;
}

void building_slot::change_production(const production_type *production_type, const int multiplier, const bool change_input_storage)
{
	const int old_output = this->get_production_type_output(production_type);

	const int change = production_type->get_output_value() * multiplier;
	this->employed_capacity += change;

	const int changed_production_type_employed_capacity = (this->production_type_employed_capacities[production_type] += change);
	assert_throw(changed_production_type_employed_capacity >= 0);
	if (changed_production_type_employed_capacity == 0) {
		this->production_type_employed_capacities.erase(production_type);
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		if (input_commodity->is_storable() && change_input_storage) {
			country_game_data->change_stored_commodity(input_commodity, -input_value * multiplier);
		}
		country_game_data->change_commodity_input(input_commodity, input_value * multiplier);
	}

	const int new_output = this->get_production_type_output(production_type);
	country_game_data->change_commodity_output(production_type->get_output_commodity(), new_output - old_output);
}

bool building_slot::can_increase_production(const production_type *production_type) const
{
	assert_throw(this->get_building() != nullptr);
	assert_throw(vector::contains(this->get_building()->get_production_types(), production_type));

	if ((this->get_employed_capacity() + production_type->get_output_value()) > this->get_capacity()) {
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
	try {
		assert_throw(this->can_increase_production(production_type));

		this->change_production(production_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error increasing production of \"" + production_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
	}
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

void building_slot::decrease_production(const production_type *production_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_production(production_type));

		this->change_production(production_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error decreasing production of \"" + production_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
	}
}

void building_slot::apply_country_modifier(const metternich::country *country, const int multiplier)
{
	if (this->get_building() != nullptr && this->get_building()->get_country_modifier() != nullptr) {
		this->get_building()->get_country_modifier()->apply(country, multiplier);
	}
}

}
