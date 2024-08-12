#include "metternich.h"

#include "unit/military_unit_type.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "technology/technology.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_domain.h"
#include "unit/promotion.h"
#include "util/assert_util.h"

namespace metternich {

void military_unit_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "stats") {
		scope.for_each_property([&](const gsml_property &property) {
			const military_unit_stat stat = enum_converter<military_unit_stat>::to_enum(property.get_key());
			const centesimal_int stat_value(property.get_value());
			this->stats[stat] = stat_value;
		});
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "free_promotions") {
		for (const std::string &value : values) {
			this->free_promotions.push_back(promotion::get(value));
		}
	} else if (tag == "upgrades") {
		for (const std::string &value : values) {
			this->upgrades.insert(military_unit_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void military_unit_type::initialize()
{
	assert_throw(this->unit_class != nullptr);

	this->unit_class->add_unit_type(this);

	if (!this->get_unit_class()->is_animal()) {
		if (this->culture != nullptr) {
			this->culture->set_military_class_unit_type(this->get_unit_class(), this);
		} else if (this->cultural_group != nullptr) {
			assert_throw(this->cultural_group->get_military_class_unit_type(this->get_unit_class()) == nullptr);

			this->cultural_group->set_military_class_unit_type(this->get_unit_class(), this);
		} else {
			this->unit_class->set_default_unit_type(this);
		}
	}

	if (this->required_technology != nullptr) {
		assert_throw(this->get_unit_class() != nullptr);
		this->required_technology->add_enabled_military_unit(this);
	}

	named_data_entry::initialize();
}

void military_unit_type::check() const
{
	assert_throw(this->get_domain() != military_unit_domain::none);
	assert_throw(this->get_icon() != nullptr);
}

military_unit_category military_unit_type::get_category() const
{
	if (this->get_unit_class() == nullptr) {
		return military_unit_category::none;
	}

	return this->get_unit_class()->get_category();
}

military_unit_domain military_unit_type::get_domain() const
{
	if (this->get_unit_class() == nullptr) {
		return military_unit_domain::none;
	}

	return this->get_unit_class()->get_domain();
}

bool military_unit_type::is_infantry() const
{
	switch (this->get_category()) {
		case military_unit_category::militia:
		case military_unit_category::mace_infantry:
		case military_unit_category::spear_infantry:
		case military_unit_category::blade_infantry:
		case military_unit_category::light_infantry:
		case military_unit_category::regular_infantry:
		case military_unit_category::heavy_infantry:
		case military_unit_category::bowmen:
			return true;
		default:
			return false;
	}
}

bool military_unit_type::is_cavalry() const
{
	switch (this->get_category()) {
		case military_unit_category::light_cavalry:
		case military_unit_category::heavy_cavalry:
		case military_unit_category::spear_cavalry:
			return true;
		default:
			return false;
	}
}

bool military_unit_type::is_artillery() const
{
	switch (this->get_category()) {
		case military_unit_category::light_artillery:
		case military_unit_category::heavy_artillery:
			return true;
		default:
			return false;
	}
}

bool military_unit_type::is_ship() const
{
	if (this->get_unit_class() == nullptr) {
		return false;
	}

	return this->get_unit_class()->is_ship();
}

centesimal_int military_unit_type::get_stat_for_country(const military_unit_stat stat, const country *country) const
{
	centesimal_int value = this->get_stat(stat);
	value += country->get_game_data()->get_military_unit_type_stat_modifier(this, stat);
	return value;
}

int military_unit_type::get_score() const
{
	int score = this->get_wealth_cost();

	for (const auto &[commodity, cost] : this->get_commodity_costs()) {
		if (commodity->get_base_price() == 0) {
			continue;
		}

		score += cost * commodity->get_base_price();
	}

	if (this->get_category() != military_unit_category::none && is_leader_military_unit_category(this->get_category())) {
		score += country_game_data::base_leader_cost * commodity::abstract_commodity_value;
	}

	return score;
}

}
