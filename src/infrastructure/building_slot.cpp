#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "database/defines.h"
#include "domain/country_economy.h"
#include "domain/country_technology.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/wonder.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace metternich {

building_slot::building_slot(const building_slot_type *type, const site *settlement)
	: type(type), settlement(settlement)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_settlement() != nullptr);

	connect(this, &building_slot::building_changed, this, &building_slot::domain_modifier_changed);
}

void building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	if (building != nullptr) {
		assert_throw(building->get_slot_type() == this->get_type());
	}

	const int holding_level_change = this->get_settlement()->get_game_data()->get_building_holding_level_change(building);
	const int fortification_level_change = this->get_settlement()->get_game_data()->get_building_fortification_level_change(building);

	const building_type *old_building = this->get_building();

	if (old_building != nullptr) {
		this->on_building_gained(old_building, -1);
	}

	this->building = building;

	if (this->get_building() != nullptr) {
		this->on_building_gained(this->get_building(), 1);
	}

	if (holding_level_change != 0) {
		this->get_settlement()->get_game_data()->change_holding_level(holding_level_change);
	}

	if (fortification_level_change != 0) {
		this->get_settlement()->get_game_data()->change_fortification_level(fortification_level_change);
	}

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
	const site_game_data *settlement_game_data = this->get_settlement()->get_game_data();

	if (settlement_game_data->get_culture()->get_building_class_type(building->get_building_class()) != building) {
		return false;
	}

	if (building->get_min_holding_level() > 0 && this->get_settlement()->get_max_holding_level() < building->get_min_holding_level()) {
		return false;
	}

	if (!vector::contains(building->get_holding_types(), settlement_game_data->get_holding_type())) {
		return false;
	}

	if (building->get_conditions() != nullptr) {
		if (!building->get_conditions()->check(this->get_settlement(), read_only_context(this->get_settlement()))) {
			return false;
		}
	}

	if (building->is_capitol() && !this->get_settlement()->get_game_data()->can_be_capital()) {
		return false;
	}

	return true;
}

bool building_slot::can_maintain_building(const building_type *building) const
{
	if (building->is_capitol() && !this->get_settlement()->get_game_data()->is_capital()) {
		return false;
	}

	if (building->is_provincial_capitol() && !this->get_settlement()->get_game_data()->is_provincial_capital()) {
		return false;
	}

	if (building->is_wonder_only() && this->get_wonder() == nullptr) {
		return false;
	}

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

	if (building->get_holding_level() > 0) {
		const int total_holding_level = this->get_settlement()->get_game_data()->get_holding_level() + this->get_settlement()->get_game_data()->get_building_holding_level_change(building);
		if (total_holding_level > this->get_settlement()->get_max_holding_level()) {
			return false;
		}
	}

	if (building->get_fortification_level() > 0) {
		const int total_fortification_level = this->get_settlement()->get_game_data()->get_fortification_level() + this->get_settlement()->get_game_data()->get_building_fortification_level_change(building);
		if (total_fortification_level > this->get_settlement()->get_max_holding_level()) {
			return false;
		}
	}

	if (this->get_building() != nullptr) {
		if (building == this->get_building()) {
			return false;
		}

		if (building->get_fortification_level() < this->get_building()->get_fortification_level()) {
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
	if (building->get_base_building() != nullptr && this->get_building() != building->get_base_building()) {
		return false;
	}

	if (building->get_fortification_level() > 0) {
		const int total_fortification_level = this->get_settlement()->get_game_data()->get_fortification_level() + this->get_settlement()->get_game_data()->get_building_fortification_level_change(building);
		if (total_fortification_level > this->get_settlement()->get_game_data()->get_holding_level()) {
			return false;
		}
	}

	if (building->get_min_holding_level() > 0 && this->get_settlement()->get_game_data()->get_holding_level() < building->get_min_holding_level()) {
		return false;
	}

	if (building->get_build_conditions() != nullptr) {
		if (!building->get_build_conditions()->check(this->get_settlement(), read_only_context(this->get_settlement()))) {
			return false;
		}
	}

	const country_economy *country_economy = this->get_country()->get_economy();
	const int wealth_cost = building->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0 && wealth_cost > country_economy->get_wealth()) {
		return false;
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs_for_site(this->get_settlement())) {
		if (cost > country_economy->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_gain_building(building);
}

void building_slot::on_building_gained(const building_type *building, const int multiplier)
{
	site_game_data *settlement_game_data = this->get_settlement()->get_game_data();
	settlement_game_data->on_building_gained(building, multiplier);
}

void building_slot::set_wonder(const metternich::wonder *wonder)
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

		if (this->get_building() == nullptr || this->get_wonder()->get_building()->is_any_base_building(this->get_building())) {
			this->set_building(this->get_wonder()->get_building());
		}
	}

	if (game::get()->is_running()) {
		emit wonder_changed();
	}
}

void building_slot::set_under_construction_wonder(const metternich::wonder *wonder)
{
	if (wonder == this->get_under_construction_wonder()) {
		return;
	}

	this->under_construction_wonder = wonder;

	if (game::get()->is_running()) {
		emit under_construction_wonder_changed();
	}
}

bool building_slot::can_have_wonder(const metternich::wonder *wonder) const
{
	if (!wonder->is_enabled()) {
		return false;
	}

	const site_game_data *settlement_game_data = this->get_settlement()->get_game_data();

	if (wonder->get_province_conditions() != nullptr) {
		if (!wonder->get_province_conditions()->check(settlement_game_data->get_province(), read_only_context(settlement_game_data->get_province()))) {
			return false;
		}
	}

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

bool building_slot::can_gain_wonder(const metternich::wonder *wonder) const
{
	if (this->get_wonder() != nullptr) {
		return false;
	}

	if (!this->can_have_wonder(wonder)) {
		return false;
	}

	if (wonder->get_obsolescence_technology() != nullptr && this->get_country() != nullptr && this->get_country()->get_technology()->has_technology(wonder->get_obsolescence_technology())) {
		return false;
	}

	if (game::get()->get_wonder_country(wonder) != nullptr) {
		return false;
	}

	return true;
}

bool building_slot::can_build_wonder(const metternich::wonder *wonder) const
{
	if (wonder->get_building()->get_base_building() != nullptr && this->get_building() != wonder->get_building()->get_base_building() && !this->get_building()->is_any_base_building(wonder->get_building())) {
		return false;
	}

	const country_economy *country_economy = this->get_country()->get_economy();
	const int wealth_cost = wonder->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0 && wealth_cost > country_economy->get_wealth()) {
		return false;
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs_for_country(this->get_country())) {
		if (cost > country_economy->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return this->can_gain_wonder(wonder);
}

void building_slot::build_wonder(const metternich::wonder *wonder)
{
	if (this->get_under_construction_building() != nullptr || this->get_under_construction_wonder() != nullptr) {
		this->cancel_construction();
	}

	country_economy *country_economy = this->get_country()->get_economy();
	const int wealth_cost = wonder->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_economy->change_wealth(-wealth_cost);
	}

	for (const auto &[commodity, cost] : wonder->get_commodity_costs_for_country(this->get_country())) {
		country_economy->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_wonder(wonder);
}

void building_slot::build_building(const building_type *building)
{
	if (this->get_under_construction_building() != nullptr) {
		this->cancel_construction();
	}

	country_economy *country_economy = this->get_country()->get_economy();
	const int wealth_cost = building->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_economy->change_wealth(-wealth_cost);
	}

	for (const auto &[commodity, cost] : building->get_commodity_costs_for_site(this->get_settlement())) {
		country_economy->change_stored_commodity(commodity, -cost);
	}

	this->set_under_construction_building(building);
}

void building_slot::cancel_construction()
{
	if (this->get_under_construction_wonder() != nullptr) {
		country_economy *country_economy = this->get_country()->get_economy();
		const int wealth_cost = this->get_under_construction_wonder()->get_wealth_cost_for_country(this->get_country());
		if (wealth_cost > 0) {
			country_economy->change_wealth(wealth_cost);
		}

		for (const auto &[commodity, cost] : this->get_under_construction_wonder()->get_commodity_costs_for_country(this->get_country())) {
			country_economy->change_stored_commodity(commodity, cost);
		}

		this->set_under_construction_wonder(nullptr);
		return;
	}

	if (this->get_under_construction_building() == nullptr) {
		return;
	}

	country_economy *country_economy = this->get_country()->get_economy();
	const int wealth_cost = this->get_under_construction_building()->get_wealth_cost_for_country(this->get_country());
	if (wealth_cost > 0) {
		country_economy->change_wealth(wealth_cost);
	}

	for (const auto &[commodity, cost] : this->get_under_construction_building()->get_commodity_costs_for_site(this->get_settlement())) {
		country_economy->change_stored_commodity(commodity, cost);
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

			if (!this->get_country()->get_technology()->has_technology(building->get_required_technology())) {
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

const wonder *building_slot::get_buildable_wonder() const
{
	for (const metternich::wonder *wonder : this->get_type()->get_wonders()) {
		if (wonder->get_required_technology() != nullptr) {
			if (this->get_country() == nullptr) {
				continue;
			}

			if (!this->get_country()->get_technology()->has_technology(wonder->get_required_technology())) {
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

const domain *building_slot::get_country() const
{
	return this->get_settlement()->get_game_data()->get_owner();
}

bool building_slot::is_available() const
{
	if (this->get_building() != nullptr) {
		return true;
	}

	for (const building_type *building : this->get_type()->get_building_types()) {
		if (building->get_base_building() != nullptr) {
			continue;
		}

		if (!this->can_have_building(building)) {
			continue;
		}

		if (building->is_wonder_only()) {
			bool can_have_wonder = false;

			for (const metternich::wonder *wonder : this->get_type()->get_wonders()) {
				if (wonder->get_building() != building) {
					continue;
				}

				if (wonder->get_obsolescence_technology() != nullptr && this->get_country() != nullptr && this->get_country()->get_technology()->has_technology(wonder->get_obsolescence_technology())) {
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

QString building_slot::get_modifier_string() const
{
	if (this->get_building() == nullptr) {
		return QString();
	}

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

	if (this->get_building()->get_holding_level() != 0) {
		if (!str.empty()) {
			str += "\n";
		}

		const QColor &number_color = this->get_building()->get_holding_level() < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		str += std::format("Holding Level: {}", string::colored(number::to_signed_string(this->get_building()->get_holding_level()), number_color));
	}

	if (this->get_building()->get_fortification_level() != 0) {
		if (!str.empty()) {
			str += "\n";
		}

		const QColor &number_color = this->get_building()->get_fortification_level() < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		str += std::format("Fortification Level: {}", string::colored(number::to_signed_string(this->get_building()->get_fortification_level()), number_color));
	}

	std::string site_modifier_str;
	if (this->get_building()->get_modifier() != nullptr) {
		site_modifier_str += this->get_building()->get_modifier()->get_string(this->get_settlement());
	}

	const commodity_map<int> building_commodity_bonuses = this->get_country()->get_economy()->get_building_commodity_bonuses(this->get_building());
	for (const auto &[commodity, bonus] : building_commodity_bonuses) {
		const std::string base_string = commodity->is_storable() ? std::format("{} Output: ", commodity->get_name()) : std::format("{}: ", commodity->get_name());

		const size_t find_pos = site_modifier_str.find(base_string);
		if (find_pos != std::string::npos) {
			const size_t number_start_pos = site_modifier_str.find('>', find_pos) + 2;
			const size_t number_end_pos = site_modifier_str.find('<', number_start_pos);

			if (site_modifier_str.at(number_end_pos - 1) != '%') {
				const std::string number_str = site_modifier_str.substr(number_start_pos, number_end_pos - number_start_pos);
				const int new_number = std::stoi(number_str) + bonus;
				site_modifier_str.replace(number_start_pos, number_end_pos - number_start_pos, std::to_string(new_number));

				continue;
			}
		}

		if (!site_modifier_str.empty()) {
			site_modifier_str += "\n";
		}

		const std::string number_str = number::to_signed_string(bonus);
		const QColor &number_color = bonus < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str, number_color);

		site_modifier_str += base_string + colored_number_str;
	}

	if (!site_modifier_str.empty()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += site_modifier_str;
	}

	if (this->get_building()->get_province_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_province_modifier()->get_string(province);
	}

	if (this->get_building()->get_domain_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_domain_modifier()->get_string(this->get_country());
	}

	if (this->get_building()->get_weighted_domain_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		const domain_game_data *domain_game_data = this->get_country()->get_game_data();
		const centesimal_int multiplier = centesimal_int(1) / domain_game_data->get_holding_count();
		str += this->get_building()->get_weighted_domain_modifier()->get_string(this->get_country(), multiplier, 0, false);
	}

	return QString::fromStdString(str);
}

}
