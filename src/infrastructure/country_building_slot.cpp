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

	connect(this, &building_slot::building_changed, this, &country_building_slot::available_production_types_changed);

	const country_game_data *country_game_data = this->get_country()->get_game_data();

	connect(country_game_data, &country_game_data::settlement_building_counts_changed, this, &country_building_slot::country_modifier_changed);
	connect(country_game_data, &country_game_data::provinces_changed, this, &country_building_slot::country_modifier_changed);
}


void country_building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	const building_type *old_building = this->get_building();

	if (old_building != nullptr && !old_building->is_provincial()) {
		if (old_building->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			old_building->get_country_modifier()->apply(this->get_country(), -this->get_level());
		}
	}

	building_slot::set_building(building);

	this->set_expanding(false);

	if (this->get_building() != nullptr && !this->get_building()->is_provincial()) {
		if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			this->get_building()->get_country_modifier()->apply(this->get_country(), this->get_level());
		}
	}

	const int old_base_capacity = old_building ? old_building->get_base_capacity() : 0;
	const int new_base_capacity = this->get_building() ? this->get_building()->get_base_capacity() : 0;
	int capacity_change = new_base_capacity - old_base_capacity;

	if (this->get_expansion_count() > 0) {
		const int old_expanded_capacity = old_building ? (old_building->get_capacity_increment() * this->get_expansion_count()) : 0;
		const int new_expanded_capacity = this->get_building() ? (this->get_building()->get_capacity_increment() * this->get_expansion_count()) : 0;
		capacity_change += new_expanded_capacity - old_expanded_capacity;
	}

	if (capacity_change != 0) {
		this->change_capacity(capacity_change);
	}

	if (building != nullptr) {
		if (building->is_expandable()) {
			this->set_level(std::max(this->get_level(), 1));
			if (building->get_max_level() != 0) {
				this->set_level(std::min(this->get_level(), building->get_max_level()));
			}
		} else {
			this->set_level(1);
		}
	} else {
		this->set_level(0);
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
}

bool country_building_slot::can_have_building(const building_type *building) const
{
	if (building != this->get_country()->get_culture()->get_building_class_type(building->get_building_class())) {
		return false;
	}

	return building_slot::can_have_building(building);
}

bool country_building_slot::can_build_building(const building_type *building) const
{
	if (building->is_provincial()) {
		return false;
	}

	return building_slot::can_build_building(building);
}

void country_building_slot::change_expansion_count(const int change)
{
	if (change == 0) {
		return;
	}

	this->expansion_count += change;

	this->change_capacity(this->get_building()->get_capacity_increment() * change);

	if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr && !this->get_building()->is_provincial()) {
		this->get_building()->get_country_modifier()->apply(this->get_country(), change);
	}
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
	assert_throw(this->get_building() != nullptr);

	this->change_expansion_count(1);

	this->set_expanding(false);
}

void country_building_slot::change_capacity(const int change)
{
	if (change == 0) {
		return;
	}

	this->capacity += change;

	if (change < 0) {
		//clear production that is over capacity
		if (this->get_building() != nullptr && this->get_employed_capacity() > this->get_capacity()) {
			for (const production_type *production_type : this->get_building()->get_production_types()) {
				while (this->get_employed_capacity() > this->get_capacity() && this->get_production_type_employed_capacity(production_type) > 0) {
					this->decrease_production(production_type, true);
				}
			}
		}
	}

	if (game::get()->is_running()) {
		emit capacity_changed();
	}
}

std::vector<const production_type *> country_building_slot::get_available_production_types() const
{
	if (this->get_building() == nullptr) {
		return {};
	}

	std::vector<const production_type *> production_types;

	for (const production_type *production_type : this->get_building()->get_production_types()) {
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

int country_building_slot::get_production_type_input_wealth(const production_type *production_type) const
{
	if (production_type->get_input_wealth() == 0) {
		return 0;
	}

	const int employed_capacity = this->get_production_type_employed_capacity(production_type);

	if (employed_capacity == 0) {
		return 0;
	}

	return this->get_country()->get_game_data()->get_inflated_value(production_type->get_input_wealth() * employed_capacity);
}

centesimal_int country_building_slot::get_production_type_output(const production_type *production_type) const
{
	centesimal_int output(this->get_production_type_employed_capacity(production_type));

	if (output == 0) {
		return centesimal_int(0);
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();

	centesimal_int output_modifier = country_game_data->get_output_modifier() + country_game_data->get_commodity_output_modifier(production_type->get_output_commodity());
	if (production_type->is_industrial()) {
		output_modifier += country_game_data->get_industrial_output_modifier();
	}

	if (output_modifier != 0) {
		output *= centesimal_int(100) + output_modifier;
		output /= 100;

		output = centesimal_int::max(output, centesimal_int(0));
	}

	return output;
}

void country_building_slot::change_production(const production_type *production_type, const int multiplier, const bool change_input_storage)
{
	const centesimal_int old_output = this->get_production_type_output(production_type);
	const commodity_map<int> old_inputs = this->get_production_type_inputs(production_type);
	const int old_input_wealth = this->get_production_type_input_wealth(production_type);

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

	const int new_input_wealth = this->get_production_type_input_wealth(production_type);
	const int input_wealth_change = new_input_wealth - old_input_wealth;
	if (change_input_storage) {
		country_game_data->change_wealth(-input_wealth_change);
	}
	country_game_data->change_wealth_income(-input_wealth_change);

	const centesimal_int new_output = this->get_production_type_output(production_type);
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

	if (production_type->get_input_wealth() != 0 && country_game_data->get_wealth_with_credit() < production_type->get_input_wealth()) {
		return false;
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

QString country_building_slot::get_country_modifier_string() const
{
	if (this->get_building() == nullptr) {
		return QString();
	}

	std::string str;

	const country_game_data *country_game_data = this->get_country()->get_game_data();

	if (this->get_building()->get_country_modifier() != nullptr) {
		centesimal_int multiplier(1);
		if (this->get_building()->is_provincial()) {
			multiplier = centesimal_int(country_game_data->get_settlement_building_count(this->get_building())) / country_game_data->get_settlement_count();
		}

		str = this->get_building()->get_country_modifier()->get_string(this->get_country(), multiplier);
	}

	if (this->get_building()->get_stackable_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		assert_throw(this->get_building()->is_provincial());
		const int multiplier = country_game_data->get_settlement_building_count(this->get_building());
		str += this->get_building()->get_stackable_country_modifier()->get_string(this->get_country(), multiplier);
	}

	return QString::fromStdString(str);
}

}
