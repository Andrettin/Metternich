#include "metternich.h"

#include "infrastructure/settlement_building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
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
#include "util/string_util.h"
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

	if (building->get_settlement_conditions() != nullptr) {
		if (!building->get_settlement_conditions()->check(this->get_settlement(), read_only_context(this->get_settlement()))) {
			return false;
		}
	}

	if (building->get_province_conditions() != nullptr) {
		if (!building->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

	if (building->is_capitol() && !this->get_settlement()->get_game_data()->can_be_capital()) {
		return false;
	}

	return building_slot::can_have_building(building);
}

bool settlement_building_slot::can_maintain_building(const building_type *building) const
{
	if (building->is_capitol() && !this->get_settlement()->get_game_data()->is_capital()) {
		return false;
	}

	if (building->is_provincial_capitol() && !this->get_settlement()->get_game_data()->is_provincial_capital()) {
		return false;
	}

	return building_slot::can_maintain_building(building);
}

bool settlement_building_slot::can_build_building(const building_type *building) const
{
	if (building->get_build_conditions() != nullptr) {
		if (!building->get_build_conditions()->check(this->get_settlement(), read_only_context(this->get_settlement()))) {
			return false;
		}
	}

	return building_slot::can_build_building(building);
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

	const site_game_data *settlement_game_data = this->get_settlement()->get_game_data();

	if (wonder->get_province_conditions() != nullptr) {
		if (!wonder->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

	return this->can_have_building(wonder->get_building());
}

bool settlement_building_slot::can_gain_wonder(const metternich::wonder *wonder) const
{
	if (!this->can_have_wonder(wonder)) {
		return false;
	}

	if (this->get_wonder() != nullptr) {
		return false;
	}

	return true;
}

bool settlement_building_slot::can_build_wonder(const metternich::wonder *wonder) const
{
	if (wonder->get_building()->get_required_building() != nullptr && this->get_building() != wonder->get_building()->get_required_building() && !this->get_building()->is_any_required_building(wonder->get_building())) {
		return false;
	}

	const country_game_data *country_game_data = this->get_country()->get_game_data();
	const int wealth_cost = wonder->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0 && country_game_data->get_inflated_value(wealth_cost) > country_game_data->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs_for_country(this->get_country())) {
		if (cost > country_game_data->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_gain_wonder(wonder);
}

void settlement_building_slot::build_wonder(const metternich::wonder *wonder)
{
	if (this->get_under_construction_building() != nullptr || this->get_under_construction_wonder() != nullptr) {
		this->cancel_construction();
	}

	country_game_data *country_game_data = this->get_country()->get_game_data();
	const int wealth_cost = wonder->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_game_data->change_wealth_inflated(-wealth_cost);
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs_for_country(this->get_country())) {
		country_game_data->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_wonder(wonder);
}

void settlement_building_slot::cancel_construction()
{
	if (this->get_under_construction_wonder() != nullptr) {
		country_game_data *country_game_data = this->get_country()->get_game_data();
		const int wealth_cost = this->get_under_construction_wonder()->get_wealth_cost_for_country(this->get_country());
		if (wealth_cost > 0) {
			country_game_data->change_wealth(wealth_cost);
		}

		for (const auto &[commodity, cost] : this->get_under_construction_wonder()->get_commodity_costs_for_country(this->get_country())) {
			country_game_data->change_stored_commodity(commodity, cost);
		}

		this->set_under_construction_wonder(nullptr);
		return;
	}

	building_slot::cancel_construction();
}

const wonder *settlement_building_slot::get_buildable_wonder() const
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

		return wonder;
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

	if (this->get_building()->get_settlement_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_settlement_modifier()->get_string(this->get_settlement());
	}

	const commodity_map<int> building_commodity_bonuses = this->get_country()->get_game_data()->get_building_commodity_bonuses(this->get_building());
	for (const auto &[commodity, bonus] : building_commodity_bonuses) {
		if (!str.empty()) {
			str += "\n";
		}

		const std::string number_str = number::to_signed_string(bonus);
		const QColor &number_color = bonus < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str, number_color);

		str += std::format("{} Output: {}", commodity->get_name(), colored_number_str);
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
