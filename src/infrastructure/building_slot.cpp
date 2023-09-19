#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
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

		if (building->get_score() < this->get_building()->get_score()) {
			return false;
		}

		if (building->get_score() == this->get_building()->get_score()) {
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
	if (building->get_wealth_cost() > 0 && building->get_wealth_cost() > country_game_data->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs()) {
		if (cost > country_game_data->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_gain_building(building);
}

void building_slot::build_building(const building_type *building)
{
	if (this->get_under_construction_building() != nullptr) {
		this->cancel_construction();
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	if (building->get_wealth_cost() > 0) {
		country_game_data->change_wealth(-building->get_wealth_cost());
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs()) {
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
	if (this->get_under_construction_building()->get_wealth_cost() > 0) {
		country_game_data->change_wealth(this->get_under_construction_building()->get_wealth_cost());
	}

	for (const auto &[commodity, cost] : this->get_under_construction_building()->get_commodity_costs()) {
		country_game_data->change_stored_commodity(commodity, cost);
	}

	this->set_under_construction_building(nullptr);
}

building_type *building_slot::get_buildable_building() const
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

		return const_cast<building_type *>(building);
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

		return true;
	}

	return false;
}

}
