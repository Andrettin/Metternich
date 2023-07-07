#include "metternich.h"

#include "infrastructure/building_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/condition/capital_condition.h"
#include "script/factor.h"
#include "script/modifier.h"
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

	if (tag == "production_types") {
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
	} else if (tag == "province_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		database::process_gsml_data(conditions, scope);
		this->province_conditions = std::move(conditions);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		database::process_gsml_data(this->province_modifier, scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else if (tag == "stackable_country_modifier") {
		this->stackable_country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->stackable_country_modifier, scope);
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
		if (this->get_province_conditions() == nullptr) {
			this->province_conditions = std::make_unique<and_condition<province>>();
		}

		this->province_conditions->add_condition(std::make_unique<capital_condition>(true));
	}

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

	if (this->get_required_building() == this) {
		throw std::runtime_error(std::format("Building type \"{}\" requires itself.", this->get_identifier()));
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

QVariantList building_type::get_production_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_production_types());
}

int building_type::get_score() const
{
	int score = building_type::base_score * std::max(1, this->get_base_capacity());

	score += this->get_fort_level() * 10;

	if (this->get_country_modifier() != nullptr) {
		score += this->get_country_modifier()->get_score() / (this->is_provincial() ? 10 : 1);
	}

	if (this->get_stackable_country_modifier() != nullptr) {
		score += this->get_stackable_country_modifier()->get_score();
	}

	if (this->get_province_modifier() != nullptr) {
		score += this->get_province_modifier()->get_score();
	}

	return score;
}

}
