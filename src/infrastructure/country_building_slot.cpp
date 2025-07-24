#include "metternich.h"

#include "infrastructure/country_building_slot.h"

#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "infrastructure/building_type.h"
#include "population/education_type.h"
#include "population/population.h"
#include "population/population_type.h"
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

	connect(this, &building_slot::building_changed, this, &country_building_slot::available_education_types_changed);

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
			old_building->get_country_modifier()->remove(this->get_country());
		}
	}

	building_slot::set_building(building);

	if (this->get_building() != nullptr && !this->get_building()->is_provincial()) {
		if (this->get_building()->get_country_modifier() != nullptr && this->get_country() != nullptr) {
			this->get_building()->get_country_modifier()->apply(this->get_country());
		}
	}

	//clear education for education types which are no longer valid
	if (old_building != nullptr && !old_building->get_education_types().empty()) {
		if (building == nullptr || building->get_education_types() != old_building->get_education_types()) {
			for (const education_type *education_type : old_building->get_education_types()) {
				if (building != nullptr && vector::contains(building->get_education_types(), education_type)) {
					continue;
				}

				while (this->get_education_type_employed_capacity(education_type) > 0) {
					this->decrease_education(education_type, true);
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

std::vector<const education_type *> country_building_slot::get_available_education_types() const
{
	if (this->get_building() == nullptr) {
		return {};
	}

	std::vector<const education_type *> education_types;

	for (const education_type *education_type : this->get_building()->get_education_types()) {
		if (!education_type->is_enabled()) {
			continue;
		}

		education_types.push_back(education_type);
	}

	return education_types;
}

QVariantList country_building_slot::get_available_education_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_education_types());
}

commodity_map<int> country_building_slot::get_education_type_inputs(const education_type *education_type) const
{
	commodity_map<int> inputs;

	for (const auto &[input_commodity, input_value] : education_type->get_input_commodities()) {
		//ensure each input commodity for the education type is in the map
		inputs[input_commodity] = 0;
	}

	const int employed_capacity = this->get_education_type_employed_capacity(education_type);

	if (employed_capacity == 0) {
		return inputs;
	}

	for (const auto &[input_commodity, input_value] : education_type->get_input_commodities()) {
		const int total_input = input_value * employed_capacity;
		inputs[input_commodity] = total_input;
	}

	return inputs;
}

QVariantList country_building_slot::get_education_type_inputs(metternich::education_type *education_type) const
{
	const metternich::education_type *const_education_type = education_type;
	return archimedes::map::to_qvariant_list(this->get_education_type_inputs(const_education_type));
}

int country_building_slot::get_education_type_input_wealth(const education_type *education_type) const
{
	if (education_type->get_input_wealth() == 0) {
		return 0;
	}

	const int employed_capacity = this->get_education_type_employed_capacity(education_type);

	if (employed_capacity == 0) {
		return 0;
	}

	return this->get_country()->get_economy()->get_inflated_value(education_type->get_input_wealth() * employed_capacity);
}

int country_building_slot::get_education_type_output(const education_type *education_type) const
{
	return this->get_education_type_employed_capacity(education_type);
}

void country_building_slot::change_education(const education_type *education_type, const int multiplier, const bool change_input_storage)
{
	const int old_output = this->get_education_type_output(education_type);
	const commodity_map<int> old_inputs = this->get_education_type_inputs(education_type);
	const int old_input_wealth = this->get_education_type_input_wealth(education_type);

	const int change = multiplier;

	const int changed_education_type_employed_capacity = (this->education_type_employed_capacities[education_type] += change);
	assert_throw(changed_education_type_employed_capacity >= 0);
	if (changed_education_type_employed_capacity == 0) {
		this->education_type_employed_capacities.erase(education_type);
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	country_economy *country_economy = this->get_country()->get_economy();

	const commodity_map<int> new_inputs = this->get_education_type_inputs(education_type);
	for (const auto &[input_commodity, input_value] : education_type->get_input_commodities()) {
		const int old_input = old_inputs.find(input_commodity)->second;
		const int new_input = new_inputs.find(input_commodity)->second;
		const int input_change = new_input - old_input;

		country_economy->change_commodity_input(input_commodity, centesimal_int(input_change), change_input_storage);
	}

	const int new_input_wealth = this->get_education_type_input_wealth(education_type);
	const int input_wealth_change = new_input_wealth - old_input_wealth;
	if (change_input_storage) {
		country_economy->change_wealth(-input_wealth_change);
	}
	country_economy->change_wealth_income(-input_wealth_change);

	const int new_output = this->get_education_type_output(education_type);
	const int output_change = new_output - old_output;
	country_game_data->change_population_type_input(education_type->get_input_population_type(), output_change);
	country_game_data->change_population_type_output(education_type->get_output_population_type(), output_change);
}

bool country_building_slot::can_increase_education(const education_type *education_type) const
{
	assert_throw(this->get_building() != nullptr);
	assert_throw(vector::contains(this->get_building()->get_education_types(), education_type));

	const country_game_data *country_game_data = this->get_country()->get_game_data();
	const country_economy *country_economy = this->get_country()->get_economy();

	if (country_game_data->get_population()->get_type_count(education_type->get_input_population_type()) - country_game_data->get_population_type_input(education_type->get_input_population_type()) < 1) {
		return false;
	}

	for (const auto &[input_commodity, input_value] : education_type->get_input_commodities()) {
		if (input_commodity->is_storable()) {
			if (country_economy->get_stored_commodity(input_commodity) < input_value) {
				return false;
			}
		} else {
			//for non-storable commodities, like Labor, the commodity output is used directly instead of storage
			if (country_economy->get_net_commodity_output(input_commodity) < input_value) {
				return false;
			}
		}
	}

	if (education_type->get_input_wealth() != 0 && country_economy->get_wealth_with_credit() < country_economy->get_inflated_value(education_type->get_input_wealth())) {
		return false;
	}

	return true;
}

void country_building_slot::increase_education(const education_type *education_type)
{
	try {
		assert_throw(this->can_increase_education(education_type));

		this->change_education(education_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error increasing education of \"" + education_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
	}
}

bool country_building_slot::can_decrease_education(const education_type *education_type) const
{
	assert_throw(this->get_building() != nullptr);
	assert_throw(vector::contains(this->get_building()->get_education_types(), education_type));

	if (this->get_education_type_employed_capacity(education_type) == 0) {
		return false;
	}

	return true;
}

void country_building_slot::decrease_education(const education_type *education_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_education(education_type));

		this->change_education(education_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error decreasing education of \"" + education_type->get_identifier() + "\" for country \"" + this->country->get_identifier() + "\"."));
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
		int multiplier = 1;
		if (this->get_building()->is_provincial()) {
			multiplier = country_game_data->get_settlement_building_count(this->get_building());
		}

		str = this->get_building()->get_country_modifier()->get_string(this->get_country(), multiplier);
	}

	if (this->get_building()->get_weighted_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		assert_throw(this->get_building()->is_provincial());
		centesimal_int multiplier(1);
		if (this->get_building()->is_provincial()) {
			multiplier = centesimal_int(country_game_data->get_settlement_building_count(this->get_building())) / country_game_data->get_settlement_count();
		}

		str = this->get_building()->get_weighted_country_modifier()->get_string(this->get_country(), multiplier);
	}

	return QString::fromStdString(str);
}

}
