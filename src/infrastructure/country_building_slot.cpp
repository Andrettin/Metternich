#include "metternich.h"

#include "infrastructure/country_building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_type.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/vector_util.h"

namespace metternich {

country_building_slot::country_building_slot(const building_slot_type *type, const metternich::country *country)
	: building_slot(type), country(country)
{
	assert_throw(this->get_country() != nullptr);

	connect(this, &building_slot::building_changed, this, &country_building_slot::capacity_changed);
	connect(this, &building_slot::building_changed, this, &country_building_slot::available_production_types_changed);
}


void country_building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();

	const building_type *old_building = this->get_building();

	if (old_building != nullptr && !old_building->is_provincial()) {
		country_game_data->on_building_gained(old_building, -1);

		if (old_building->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			old_building->get_country_modifier()->apply(this->get_country(), -this->get_level());
		}
	}

	building_slot::set_building(building);

	if (building != nullptr) {
		if (building->is_expandable()) {
			this->level = std::max(this->get_level(), 1);
			if (building->get_max_level() != 0) {
				this->level = std::min(this->get_level(), building->get_max_level());
			}
		} else {
			this->level = 1;
		}
	} else {
		this->level = 0;
	}
	this->set_expanding(false);

	if (this->get_building() != nullptr && !this->get_building()->is_provincial()) {
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

	//clear production that is over capacity
	if (building != nullptr && this->get_employed_capacity() > this->get_capacity()) {
		for (const production_type *production_type : building->get_production_types()) {
			while (this->get_employed_capacity() > this->get_capacity() && this->get_production_type_employed_capacity(production_type) > 0) {
				this->decrease_production(production_type, true);
			}
		}
	}
}

bool country_building_slot::can_have_building(const building_type *building) const
{
	if (building != this->get_country()->get_culture()->get_building_class_type(building->get_building_class())) {
		return false;
	}

	return building_slot::can_have_building(building);
}

bool country_building_slot::can_expand() const
{
	if (this->get_building() == nullptr) {
		return false;
	}

	if (!this->get_building()->is_expandable()) {
		return false;
	}

	if (this->get_building()->get_max_level() != 0 && this->get_level() >= this->get_building()->get_max_level()) {
		return false;
	}

	return true;
}

void country_building_slot::expand()
{
	assert_throw(this->is_expanding());

	this->level += 1;
	emit capacity_changed();

	if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
		this->get_building()->get_country_modifier()->apply(this->get_country(), 1);
	}

	this->set_expanding(false);
}

int country_building_slot::get_capacity() const
{
	if (this->get_building() != nullptr) {
		return this->get_building()->get_base_capacity() + this->get_building()->get_capacity_increment() * (this->get_level() - 1);
	}

	return 0;
}

std::vector<const production_type *> country_building_slot::get_available_production_types() const
{
	if (this->get_building() == nullptr) {
		return {};
	}

	std::vector<const production_type *> production_types;

	for (const production_type *production_type : this->get_building()->get_production_types()) {
		if (production_type->get_output_commodity() == defines::get()->get_advisor_commodity() && !game::get()->get_rules()->are_advisors_enabled()) {
			continue;
		}

		if (production_type->get_required_technology() != nullptr && !this->country->get_game_data()->has_technology(production_type->get_required_technology())) {
			continue;
		}

		production_types.push_back(production_type);
	}

	return production_types;
}

QVariantList country_building_slot::get_available_production_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_production_types());
}

commodity_map<int> country_building_slot::get_production_type_inputs(const production_type *production_type) const
{
	commodity_map<int> inputs;

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		//ensure each input commodity for the production type is in the map
		inputs[input_commodity] = 0;
	}

	const int employed_capacity = this->get_production_type_employed_capacity(production_type);

	if (employed_capacity == 0) {
		return inputs;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();

	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		int total_input = input_value * employed_capacity;

		if (input_commodity->is_labor()) {
			const int throughput_modifier = country_game_data->get_throughput_modifier() + country_game_data->get_commodity_throughput_modifier(production_type->get_output_commodity());

			if (throughput_modifier != 0) {
				assert_throw(throughput_modifier > -100);

				const centesimal_int modified_input = centesimal_int(total_input) * 100 / (100 + throughput_modifier);

				total_input = modified_input.to_int();
				if (modified_input.get_fractional_value()) {
					total_input += 1;
				}

				total_input = std::max(total_input, 1);
			}
		}

		inputs[input_commodity] = total_input;
	}

	return inputs;
}

QVariantList country_building_slot::get_production_type_inputs(metternich::production_type *production_type) const
{
	const metternich::production_type *const_production_type = production_type;
	return archimedes::map::to_qvariant_list(this->get_production_type_inputs(const_production_type));
}

int country_building_slot::get_production_type_output(const production_type *production_type) const
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

void country_building_slot::change_production(const production_type *production_type, const int multiplier, const bool change_input_storage)
{
	const int old_output = this->get_production_type_output(production_type);
	const commodity_map<int> old_inputs = this->get_production_type_inputs(production_type);

	const int change = production_type->get_output_value() * multiplier;
	this->employed_capacity += change;

	const int changed_production_type_employed_capacity = (this->production_type_employed_capacities[production_type] += change);
	assert_throw(changed_production_type_employed_capacity >= 0);
	if (changed_production_type_employed_capacity == 0) {
		this->production_type_employed_capacities.erase(production_type);
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();

	const commodity_map<int> new_inputs = this->get_production_type_inputs(production_type);
	for (const auto &[input_commodity, input_value] : production_type->get_input_commodities()) {
		const int old_input = old_inputs.find(input_commodity)->second;
		const int new_input = new_inputs.find(input_commodity)->second;
		const int input_change = new_input - old_input;

		if (input_commodity->is_storable() && change_input_storage) {
			country_game_data->change_stored_commodity(input_commodity, -input_change);
		}
		country_game_data->change_commodity_input(input_commodity, input_change);
	}

	const int new_output = this->get_production_type_output(production_type);
	country_game_data->change_commodity_output(production_type->get_output_commodity(), new_output - old_output);

}

bool country_building_slot::can_increase_production(const production_type *production_type) const
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

void country_building_slot::increase_production(const production_type *production_type)
{
	try {
		assert_throw(this->can_increase_production(production_type));

		this->change_production(production_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error increasing production of \"" + production_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
	}
}

bool country_building_slot::can_decrease_production(const production_type *production_type) const
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

void country_building_slot::decrease_production(const production_type *production_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_production(production_type));

		this->change_production(production_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error decreasing production of \"" + production_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
	}
}

}
