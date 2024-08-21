#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/wonder.h"
#include "script/condition/condition.h"
#include "util/assert_util.h"

namespace metternich {

building_slot::building_slot(const building_slot_type *type)
	: type(type)
{
	assert_throw(this->get_type() != nullptr);
}

void building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	if (building != nullptr) {
		assert_throw(building->get_slot_type() == this->get_type());
	}

	this->building = building;

	if (game::get()->is_running()) {
		emit building_changed();
	}
}

void building_slot::set_under_construction_building(const building_type *building)
{
	if (building == this->get_under_construction_building()) {
		return;
	}

	this->under_construction_building = building;

	if (game::get()->is_running()) {
		emit under_construction_building_changed();
	}
}

bool building_slot::can_have_building(const building_type *building) const
{
	if (building->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!building->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	return true;
}

bool building_slot::can_maintain_building(const building_type *building) const
{
	return this->can_have_building(building);
}

bool building_slot::can_gain_building(const building_type *building) const
{
	if (building->is_wonder_only()) {
		//cannot gain the building directly, only via the construction of a wonder
		return false;
	}

	if (!this->can_have_building(building)) {
		return false;
	}

	if (this->get_building() != nullptr) {
		if (building == this->get_building()) {
			return false;
		}

		if (building->get_base_capacity() < this->get_building()->get_base_capacity()) {
			return false;
		}

		if (building->get_fort_level() < this->get_building()->get_fort_level()) {
			return false;
		}

		if (building->get_level() < this->get_building()->get_level()) {
			return false;
		}

		if (building->get_level() == this->get_building()->get_level()) {
			//the building must be better in some way
			return false;
		}
	}

	return true;
}

bool building_slot::can_build_building(const building_type *building) const
{
	if (building->get_required_building() != nullptr && this->get_building() != building->get_required_building()) {
		return false;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();
	const int wealth_cost = building->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0 && country_game_data->get_inflated_value(wealth_cost) > country_game_data->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs_for_country(this->get_country())) {
		if (cost > country_game_data->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_gain_building(building);
}

bool building_slot::can_have_wonder(const wonder *wonder) const
{
	if (wonder->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!wonder->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	return this->can_have_building(wonder->get_building());
}

void building_slot::build_building(const building_type *building)
{
	if (this->get_under_construction_building() != nullptr) {
		this->cancel_construction();
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	const int wealth_cost = building->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_game_data->change_wealth_inflated(-wealth_cost);
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs_for_country(this->get_country())) {
		country_game_data->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_building(building);
}

void building_slot::cancel_construction()
{
	if (this->get_under_construction_building() == nullptr) {
		return;
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	const int wealth_cost = this->get_under_construction_building()->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_game_data->change_wealth(wealth_cost);
	}

	for (const auto &[commodity, cost] : this->get_under_construction_building()->get_commodity_costs_for_country(this->get_country())) {
		country_game_data->change_stored_commodity(commodity, cost);
	}

	this->set_under_construction_building(nullptr);
}

const building_type *building_slot::get_buildable_building() const
{
	for (const building_type *building : this->get_type()->get_building_types()) {
		if (building->get_required_technology() != nullptr) {
			if (this->get_country() == nullptr) {
				continue;
			}

			if (!this->get_country()->get_game_data()->has_technology(building->get_required_technology())) {
				continue;
			}
		}

		if (!this->can_build_building(building)) {
			continue;
		}

		return building;
	}

	return nullptr;
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

		if (!this->can_have_building(building)) {
			continue;
		}

		if (building->is_wonder_only()) {
			bool can_have_wonder = false;

			for (const wonder *wonder : this->get_type()->get_wonders()) {
				if (wonder->get_building() != building) {
					continue;
				}

				if (wonder->get_obsolescence_technology() != nullptr && this->get_country() != nullptr && this->get_country()->get_game_data()->has_technology(wonder->get_obsolescence_technology())) {
					continue;
				}

				if (game::get()->get_wonder_country(wonder) != nullptr) {
					continue;
				}
				//FIXME: should check whether any other country has the wonder; if so, the wonder cannot be gained and as such it should not be counted for the purposes of building slot availability

				if (!this->can_have_wonder(wonder)) {
					continue;
				}

				can_have_wonder = true;
				break;
			}

			if (!can_have_wonder) {
				continue;
			}
		}

		return true;
	}

	return false;
}

}
