#include "metternich.h"

#include "infrastructure/settlement_building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/country_building_slot.h"
#include "infrastructure/wonder.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

settlement_building_slot::settlement_building_slot(const building_slot_type *type, const site *settlement)
	: building_slot(type), settlement(settlement)
{
	assert_throw(this->get_settlement() != nullptr);

	connect(this, &building_slot::building_changed, this, &settlement_building_slot::country_modifier_changed);
}


void settlement_building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	site_game_data *settlement_game_data = this->get_settlement()->get_game_data();

	const building_type *old_building = this->get_building();

	if (old_building != nullptr) {
		settlement_game_data->on_building_gained(old_building, -1);
	}

	building_slot::set_building(building);

	if (this->get_building() != nullptr) {
		settlement_game_data->on_building_gained(this->get_building(), 1);
	}
}

bool settlement_building_slot::can_have_building(const building_type *building) const
{
	if (!building->is_provincial()) {
		return false;
	}

	const site_game_data *settlement_game_data = this->get_settlement()->get_game_data();
	if (!vector::contains(building->get_settlement_types(), settlement_game_data->get_settlement_type())) {
		return false;
	}

	if (building->get_province_conditions() != nullptr) {
		if (!building->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

	return building_slot::can_have_building(building);
}

void settlement_building_slot::set_wonder(const metternich::wonder *wonder)
{
	if (wonder == this->get_wonder()) {
		return;
	}

	if (wonder != nullptr) {
		assert_throw(wonder->get_building()->get_slot_type() == this->get_type());
	}

	site_game_data *settlement_game_data = this->get_settlement()->get_game_data();

	if (this->get_wonder() != nullptr) {
		settlement_game_data->on_wonder_gained(this->get_wonder(), -1);
	}

	this->wonder = wonder;

	if (this->get_wonder() != nullptr) {
		settlement_game_data->on_wonder_gained(this->get_wonder(), 1);

		if (this->get_building() == nullptr || this->get_wonder()->get_building()->is_any_required_building(this->get_building())) {
			this->set_building(this->get_wonder()->get_building());
		}
	}

	if (game::get()->is_running()) {
		emit wonder_changed();
	}
}

void settlement_building_slot::set_under_construction_wonder(const metternich::wonder *wonder)
{
	if (wonder == this->get_under_construction_wonder()) {
		return;
	}

	this->under_construction_wonder = wonder;

	if (game::get()->is_running()) {
		emit under_construction_wonder_changed();
	}
}

bool settlement_building_slot::can_have_wonder(const metternich::wonder *wonder) const
{
	if (this->get_wonder() != nullptr) {
		return false;
	}

	if (wonder->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!wonder->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	const site_game_data *settlement_game_data = settlement->get_game_data();

	if (wonder->get_province_conditions() != nullptr) {
		if (!wonder->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

	if (!vector::contains(wonder->get_building()->get_settlement_types(), settlement_game_data->get_settlement_type())) {
		return false;
	}

	if (wonder->get_building()->get_province_conditions() != nullptr) {
		if (!wonder->get_building()->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

	if (wonder->get_building()->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!wonder->get_building()->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	if (this->get_wonder() != nullptr) {
		if (wonder == this->get_wonder()) {
			return false;
		}

		if (wonder->get_score() < this->get_wonder()->get_score()) {
			return false;
		}

		if (wonder->get_score() == this->get_wonder()->get_score()) {
			//the wonder must be better in some way
			return false;
		}
	}

	return true;
}

bool settlement_building_slot::can_build_wonder(const metternich::wonder *wonder) const
{
	if (wonder->get_building()->get_required_building() != nullptr && this->get_building() != wonder->get_building()->get_required_building() && !this->get_building()->is_any_required_building(wonder->get_building())) {
		return false;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();
	if (wonder->get_wealth_cost() > 0 && wonder->get_wealth_cost() > country_game_data->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs()) {
		if (cost > country_game_data->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_have_wonder(wonder);
}

void settlement_building_slot::build_wonder(const metternich::wonder *wonder)
{
	if (this->get_under_construction_building() != nullptr || this->get_under_construction_wonder() != nullptr) {
		this->cancel_construction();
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	if (wonder->get_wealth_cost() > 0) {
		country_game_data->change_wealth(-wonder->get_wealth_cost());
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs()) {
		country_game_data->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_wonder(wonder);
}

void settlement_building_slot::cancel_construction()
{
	if (this->get_under_construction_wonder() != nullptr) {
		country_game_data *country_game_data = this->get_country()->get_game_data();
		if (this->get_under_construction_wonder()->get_wealth_cost() > 0) {
			country_game_data->change_wealth(this->get_under_construction_wonder()->get_wealth_cost());
		}

		for (const auto &[commodity, cost] : this->get_under_construction_wonder()->get_commodity_costs()) {
			country_game_data->change_stored_commodity(commodity, cost);
		}

		this->set_under_construction_wonder(nullptr);
		return;
	}

	building_slot::cancel_construction();
}

wonder *settlement_building_slot::get_buildable_wonder() const
{
	for (const metternich::wonder *wonder : this->get_type()->get_wonders()) {
		if (wonder->get_required_technology() != nullptr) {
			if (this->get_country() == nullptr) {
				continue;
			}

			if (!this->get_country()->get_game_data()->has_technology(wonder->get_required_technology())) {
				continue;
			}
		}

		if (!this->can_build_wonder(wonder)) {
			continue;
		}

		return const_cast<metternich::wonder *>(wonder);
	}

	return nullptr;
}

const country *settlement_building_slot::get_country() const
{
	return this->get_settlement()->get_game_data()->get_owner();
}

QString settlement_building_slot::get_modifier_string() const
{
	if (this->get_building() == nullptr) {
		return QString();
	}

	assert_throw(this->get_building()->is_provincial());

	std::string str;

	const province *province = this->get_settlement()->get_game_data()->get_province();

	if (this->get_wonder() != nullptr) {
		if (this->get_wonder()->get_country_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += this->get_wonder()->get_country_modifier()->get_string(this->get_country());
		}

		if (this->get_wonder()->get_province_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += this->get_wonder()->get_province_modifier()->get_string(province);
		}
	}

	if (this->get_building()->get_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		const country_game_data *country_game_data = this->get_country()->get_game_data();
		const centesimal_int multiplier = centesimal_int(1) / country_game_data->get_settlement_count();
		str += this->get_building()->get_country_modifier()->get_string(this->get_country(), multiplier, 0, false);
	}

	if (this->get_building()->get_stackable_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_stackable_country_modifier()->get_string(this->get_country());
	}

	if (this->get_building()->get_province_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_province_modifier()->get_string(province);
	}

	return QString::fromStdString(str);
}

}