#include "metternich.h"

#include "infrastructure/building_type.h"

#include "domain/cultural_group.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/holding_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/condition/capital_condition.h"
#include "script/condition/or_condition.h"
#include "script/condition/provincial_capital_condition.h"
#include "script/effect/capital_effect.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "script/modifier_effect/commodity_bonus_modifier_effect.h"
#include "technology/technology.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit_category.h"
#include "unit/transporter_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

building_type::building_type(const std::string &identifier) : named_data_entry(identifier)
{
}

building_type::~building_type()
{
}

void building_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "holding_types") {
		for (const std::string &value : values) {
			this->holding_types.push_back(holding_type::get(value));
		}
	} else if (tag == "recruited_civilian_unit_types") {
		for (const std::string &value : values) {
			this->recruited_civilian_unit_types.push_back(civilian_unit_type::get(value));
		}
	} else if (tag == "recruited_military_unit_categories") {
		for (const std::string &value : values) {
			this->recruited_military_unit_categories.push_back(magic_enum::enum_cast<military_unit_category>(value).value());
		}
	} else if (tag == "recruited_transporter_categories") {
		for (const std::string &value : values) {
			this->recruited_transporter_categories.push_back(magic_enum::enum_cast<transporter_category>(value).value());
		}
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<domain>>(100);
		factor->process_gsml_data(scope);
		this->cost_factor = std::move(factor);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "build_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->build_conditions = std::move(conditions);
	} else if (tag == "free_on_start_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->free_on_start_conditions = std::move(conditions);
	} else if (tag == "settlement_modifier") {
		this->settlement_modifier = std::make_unique<modifier<const site>>();
		this->settlement_modifier->process_gsml_data(scope);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		this->province_modifier->process_gsml_data(scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const domain>>();
		this->country_modifier->process_gsml_data(scope);
	} else if (tag == "weighted_country_modifier") {
		this->weighted_country_modifier = std::make_unique<modifier<const domain>>();
		this->weighted_country_modifier->process_gsml_data(scope);
	} else if (tag == "effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const site>>();
		effect_list->process_gsml_data(scope);
		this->effects = std::move(effect_list);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void building_type::initialize()
{
	assert_throw(this->building_class != nullptr);
	this->building_class->add_building_type(this);
	this->building_class->get_slot_type()->add_building_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_building_class_type(this->get_building_class()) == nullptr);

		this->culture->set_building_class_type(this->get_building_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_building_class_type(this->get_building_class()) == nullptr);

		this->cultural_group->set_building_class_type(this->get_building_class(), this);
	} else {
		this->building_class->set_default_building_type(this);
	}

	if (this->required_building != nullptr) {
		this->required_building->requiring_buildings.push_back(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_building(this);
	}

	if (this->is_capital_only()) {
		if (this->get_conditions() == nullptr) {
			this->conditions = std::make_unique<and_condition<site>>();
		}

		this->conditions->add_condition(std::make_unique<capital_condition<site>>(true));
	}

	if (this->is_provincial_capital_only()) {
		if (this->get_conditions() == nullptr) {
			this->conditions = std::make_unique<and_condition<site>>();
		}

		this->conditions->add_condition(std::make_unique<provincial_capital_condition<site>>(true));
	}

	if (this->is_capitol()) {
		this->capital_only = true;

		if (this->get_effects() == nullptr) {
			this->effects = std::make_unique<metternich::effect_list<const site>>();
		}

		this->effects->add_effect(std::make_unique<capital_effect<const site>>(true));
	}

	if (this->is_provincial_capitol()) {
		this->provincial_capital_only = true;
	}

	this->calculate_level();

	named_data_entry::initialize();
}

void building_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
	assert_throw(this->get_icon() != nullptr);

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_build_conditions() != nullptr) {
		this->get_build_conditions()->check_validity();
	}

	if (this->get_free_on_start_conditions() != nullptr) {
		this->get_free_on_start_conditions()->check_validity();
	}

	if (this->get_required_building() == this) {
		throw std::runtime_error(std::format("Building type \"{}\" requires itself.", this->get_identifier()));
	}

	if (this->get_holding_types().empty()) {
		throw std::runtime_error(std::format("Building type \"{}\" does not have any holding types listed for it.", this->get_identifier()));
	}
}

const building_slot_type *building_type::get_slot_type() const
{
	return this->get_building_class()->get_slot_type();
}

void building_type::calculate_level()
{
	if (this->required_building != nullptr) {
		if (this->required_building->get_level() == 0) {
			this->required_building->initialize();
		}

		this->level = this->required_building->get_level() + 1;
	} else {
		this->level = 1;
	}
}

QVariantList building_type::get_recruited_civilian_unit_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_recruited_civilian_unit_types());
}

QVariantList building_type::get_recruited_military_unit_categories_qvariant_list() const
{
	return container::to_qvariant_list(this->get_recruited_military_unit_categories());
}

QVariantList building_type::get_recruited_transporter_categories_qvariant_list() const
{
	return container::to_qvariant_list(this->get_recruited_transporter_categories());
}

int building_type::get_wealth_cost_for_country(const domain *domain) const
{
	int cost = this->get_wealth_cost();

	if (cost > 0) {
		if (domain->get_game_data()->get_building_cost_efficiency_modifier() != 0) {
			const int cost_efficiency_modifier = domain->get_game_data()->get_building_cost_efficiency_modifier() + domain->get_game_data()->get_building_class_cost_efficiency_modifier(this->get_building_class());
			if (cost_efficiency_modifier >= 0) {
				cost *= 100;
				cost /= 100 + cost_efficiency_modifier;
			} else {
				cost *= 100 + std::abs(cost_efficiency_modifier);
				cost /= 100;
			}
		}

		if (this->get_cost_factor() != nullptr) {
			cost = this->get_cost_factor()->calculate(domain, centesimal_int(cost)).to_int();
		}

		cost = std::max(1, cost);
	}

	return cost;
}

commodity_map<int> building_type::get_commodity_costs_for_country(const domain *domain) const
{
	commodity_map<int> costs = this->get_commodity_costs();

	for (auto &[commodity, cost] : costs) {
		if (cost > 0) {
			if (domain->get_game_data()->get_building_cost_efficiency_modifier() != 0) {
				const int cost_efficiency_modifier = domain->get_game_data()->get_building_cost_efficiency_modifier() + domain->get_game_data()->get_building_class_cost_efficiency_modifier(this->get_building_class());
				if (cost_efficiency_modifier >= 0) {
					cost *= 100;
					cost /= 100 + cost_efficiency_modifier;
				} else {
					cost *= 100 + std::abs(cost_efficiency_modifier);
					cost /= 100;
				}
			}

			if (this->get_cost_factor() != nullptr) {
				cost = this->get_cost_factor()->calculate(domain, centesimal_int(cost)).to_int();
			}

			cost = std::max(1, cost);
		}
	}

	return costs;
}

QString building_type::get_effects_string(metternich::site *site) const
{
	assert_throw(site->is_settlement());

	if (this->get_effects() != nullptr) {
		return QString::fromStdString(this->get_effects()->get_effects_string(site, read_only_context(site)));
	}

	return QString();
}

}
