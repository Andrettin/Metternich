#include "metternich.h"

#include "infrastructure/building_type.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/settlement_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
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
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

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

	if (tag == "settlement_types") {
		for (const std::string &value : values) {
			this->settlement_types.push_back(settlement_type::get(value));
		}
	} else if (tag == "production_types") {
		for (const std::string &value : values) {
			this->production_types.push_back(production_type::get(value));
		}
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<country>>(100);
		database::process_gsml_data(factor, scope);
		this->cost_factor = std::move(factor);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "settlement_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		database::process_gsml_data(conditions, scope);
		this->settlement_conditions = std::move(conditions);
	} else if (tag == "province_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		database::process_gsml_data(conditions, scope);
		this->province_conditions = std::move(conditions);
	} else if (tag == "build_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		database::process_gsml_data(conditions, scope);
		this->build_conditions = std::move(conditions);
	} else if (tag == "settlement_modifier") {
		this->settlement_modifier = std::make_unique<modifier<const site>>();
		database::process_gsml_data(this->settlement_modifier, scope);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		database::process_gsml_data(this->province_modifier, scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else if (tag == "stackable_country_modifier") {
		this->stackable_country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->stackable_country_modifier, scope);
	} else if (tag == "effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const site>>();
		database::process_gsml_data(effect_list, scope);
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
		if (this->get_settlement_conditions() == nullptr) {
			this->settlement_conditions = std::make_unique<and_condition<site>>();
		}

		this->settlement_conditions->add_condition(std::make_unique<capital_condition<site>>(true));
	}

	if (this->is_provincial_capital_only()) {
		if (this->get_settlement_conditions() == nullptr) {
			this->settlement_conditions = std::make_unique<and_condition<site>>();
		}

		this->settlement_conditions->add_condition(std::make_unique<provincial_capital_condition<site>>(true));
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

	if (this->get_province_conditions() != nullptr) {
		this->get_province_conditions()->check_validity();
	}

	if (this->get_settlement_conditions() != nullptr) {
		this->get_settlement_conditions()->check_validity();
	}

	if (this->get_build_conditions() != nullptr) {
		this->get_build_conditions()->check_validity();
	}

	if (this->get_required_building() == this) {
		throw std::runtime_error(std::format("Building type \"{}\" requires itself.", this->get_identifier()));
	}

	if (this->is_provincial() && this->get_settlement_types().empty()) {
		throw std::runtime_error(std::format("Building type \"{}\" is provincial, but does not have any settlement types listed for it.", this->get_identifier()));
	}

	if (!this->is_provincial() && !this->get_settlement_types().empty()) {
		throw std::runtime_error(std::format("Building type \"{}\" is not provincial, but does have settlement types listed for it.", this->get_identifier()));
	}

	if (this->get_employment_profession() != nullptr && this->get_employment_capacity() == 0) {
		throw std::runtime_error(std::format("Building type \"{}\" has an employment profession, but no employment capacity.", this->get_identifier()));
	}

	if (this->get_employment_capacity() > 0 && this->get_employment_profession() == nullptr) {
		throw std::runtime_error(std::format("Building type \"{}\" has an employment capacity, but no employment profession.", this->get_identifier()));
	}

	if (!this->get_production_types().empty() && !this->is_provincial()) {
		assert_throw(this->get_base_capacity() > 0);
	}

	if (this->is_expandable() && this->get_base_capacity() > 0 && this->get_capacity_increment() == 0) {
		throw std::runtime_error(std::format("Building type \"{}\" is expandable and has a base capacity, but has no capacity increment.", this->get_identifier()));
	}

	if (this->get_max_level() > 1 && !this->is_expandable()) {
		throw std::runtime_error(std::format("Building type \"{}\" has a maximum level greater than 1, but is not expandable.", this->get_identifier()));
	}

	if (this->get_province_modifier() != nullptr && !this->is_provincial()) {
		throw std::runtime_error(std::format("Building type \"{}\" has a province modifier, but is not a provincial building.", this->get_identifier()));
	}

	if (this->get_stackable_country_modifier() != nullptr && !this->is_provincial()) {
		throw std::runtime_error(std::format("Building type \"{}\" has a stackable country modifier, but is not a provincial building.", this->get_identifier()));
	}

	if (this->get_province_conditions() != nullptr && !this->is_provincial()) {
		throw std::runtime_error(std::format("Building type \"{}\" has province conditions, but is not a provincial building.", this->get_identifier()));
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

QVariantList building_type::get_production_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_production_types());
}

int building_type::get_wealth_cost_for_country(const country *country) const
{
	int cost = this->get_wealth_cost();

	if (cost > 0 && this->get_cost_factor() != nullptr) {
		cost = this->get_cost_factor()->calculate(country, centesimal_int(cost)).to_int();
		cost = std::max(1, cost);
	}

	return cost;
}

commodity_map<int> building_type::get_commodity_costs_for_country(const country *country) const
{
	commodity_map<int> costs = this->get_commodity_costs();

	for (auto &[commodity, cost] : costs) {
		if (cost > 0) {
			if (country->get_game_data()->get_building_cost_efficiency_modifier() != 0) {
				cost *= 100;
				cost /= 100 + country->get_game_data()->get_building_cost_efficiency_modifier();
			}

			if (this->get_cost_factor() != nullptr) {
				cost = this->get_cost_factor()->calculate(country, centesimal_int(cost)).to_int();
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
