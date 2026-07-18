#include "metternich.h"

#include "infrastructure/building_type.h"

#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/holding_type.h"
#include "item/item_creation_type.h"
#include "script/condition/and_condition.h"
#include "script/condition/capital_condition.h"
#include "script/condition/provincial_capital_condition.h"
#include "script/effect/capital_effect.h"
#include "script/effect/effect_list.h"
#include "script/factor.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit_category.h"
#include "unit/transporter_category.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/number_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

const std::set<std::string> building_type::database_dependencies = {
	//so that commodity units are present
	commodity::class_identifier
};

commodity_map<int64_t> building_type::commodity_weights_to_costs(const int64_t wealth_cost, const commodity_map<int> &commodity_cost_weights)
{
	commodity_map<int64_t> commodity_costs;

	int total_weight = 0;
	for (const auto &[commodity, cost_weight] : commodity_cost_weights) {
		total_weight += cost_weight;
	}

	for (const auto &[commodity, cost_weight] : commodity_cost_weights) {
		assert_throw(commodity->get_base_price() > 0);
		commodity_costs[commodity] = wealth_cost * cost_weight / total_weight / commodity->get_base_price();
	}

	return commodity_costs;
}

building_type::building_type(const std::string &identifier) : named_data_entry(identifier)
{
}

building_type::~building_type()
{
}

void building_type::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "build_duration") {
		const std::chrono::seconds duration_seconds = string::to_duration(value);
		this->build_duration = std::chrono::duration_cast<std::chrono::months>(duration_seconds);
		if ((duration_seconds % std::chrono::months(1)).count() > 0) {
			this->build_duration += std::chrono::months(1);
		}
	} else if (key == "wealth_cost") {
		this->wealth_cost = defines::get()->get_wealth_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void building_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "holding_types") {
		for (const std::string &value : values) {
			holding_type *holding_type = holding_type::get(value);
			holding_type->add_building_type(this);
			this->holding_types.push_back(holding_type);
		}
	} else if (tag == "required_buildings") {
		for (const std::string &value : values) {
			const building_type *required_building = building_type::get(value);
			this->required_buildings.push_back(required_building);
		}
	} else if (tag == "builder_civilian_unit_types") {
		for (const std::string &value : values) {
			civilian_unit_type *civilian_unit_type = civilian_unit_type::get(value);
			civilian_unit_type->add_buildable_building(this);
			this->builder_civilian_unit_types.push_back(civilian_unit_type);
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
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "commodity_cost_weights") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_cost_weights[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "cost_factor") {
		auto factor = std::make_unique<metternich::factor<site>>(100);
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
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<metternich::modifier<const province>>();
		this->province_modifier->process_gsml_data(scope);
	} else if (tag == "domain_modifier") {
		this->domain_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->domain_modifier->process_gsml_data(scope);
	} else if (tag == "weighted_domain_modifier") {
		this->weighted_domain_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->weighted_domain_modifier->process_gsml_data(scope);
	} else if (tag == "effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const site>>();
		effect_list->process_gsml_data(scope);
		this->effects = std::move(effect_list);
	} else if (tag == "item_creation_types") {
		for (const std::string &value : values) {
			this->item_creation_types.push_back(item_creation_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void building_type::initialize()
{
	assert_throw(this->building_class != nullptr);
	this->building_class->add_building_type(this);
	this->building_class->get_slot_type()->add_building_type(this);

	if (this->commodity_cost_weights.empty()) {
		this->commodity_cost_weights[defines::get()->get_construction_commodity()] = 1;
	}

	if (this->wealth_cost != 0) {
		assert_throw(this->commodity_costs.empty());

		this->commodity_costs = building_type::commodity_weights_to_costs(this->wealth_cost, this->commodity_cost_weights);

		this->wealth_cost = 0;
	}

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_building_class_type(this->get_building_class()) == nullptr);

		this->culture->set_building_class_type(this->get_building_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_building_class_type(this->get_building_class()) == nullptr);

		this->cultural_group->set_building_class_type(this->get_building_class(), this);
	} else {
		this->building_class->set_default_building_type(this);
	}

	if (this->base_building != nullptr) {
		this->base_building->derived_buildings.push_back(this);
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

	if (this->get_base_building() == this) {
		throw std::runtime_error(std::format("Building type \"{}\" is based on itself.", this->get_identifier()));
	}

	if (vector::contains(this->get_required_buildings(), this)) {
		throw std::runtime_error(std::format("Building type \"{}\" requires itself.", this->get_identifier()));
	}

	if (this->get_holding_types().empty()) {
		throw std::runtime_error(std::format("Building type \"{}\" does not have any holding types listed for it.", this->get_identifier()));
	}

	if (this->get_builder_civilian_unit_types().empty()) {
		log::log_error(std::format("Building type \"{}\" has no builder civilian unit types.", this->get_identifier()));
	}

	if (this->get_free_on_start_extra_technology().has_value() && this->get_required_technology() == nullptr) {
		throw std::runtime_error(std::format("Building type \"{}\" has a free on start extra technology value, but no required technology.", this->get_identifier()));
	}
}

const building_slot_type *building_type::get_slot_type() const
{
	return this->get_building_class()->get_slot_type();
}

void building_type::calculate_level()
{
	if (this->base_building != nullptr) {
		if (this->base_building->get_level() == 0) {
			this->base_building->initialize();
		}

		this->level = this->base_building->get_level() + 1;
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

int64_t building_type::get_population_capacity_for_province_level(const int province_level, const int province_total_holding_level) const
{
	return defines::get()->get_province_population_for_level(province_level) * this->get_holding_level() / std::max(province_total_holding_level, 1);
}

commodity_map<int64_t> building_type::get_commodity_costs_for_site(const site *site) const
{
	assert_throw(site != nullptr);

	commodity_map<int64_t> costs = this->get_commodity_costs();

	if (this->get_holding_level() > 0) {
		assert_throw(site->get_game_data()->get_holding_type() != nullptr);
		const int holding_level_change = site->get_game_data()->get_building_holding_level_change(this);

		for (int i = 0; i < holding_level_change; ++i) {
			for (const auto &[commodity, level_cost] : site->get_game_data()->get_holding_type()->get_level_commodity_costs()) {
				costs[commodity] += level_cost;
			}

			for (const auto &[commodity, level_cost_per_level] : site->get_game_data()->get_holding_type()->get_level_commodity_costs_per_level()) {
				const int64_t level_cost = level_cost_per_level * (site->get_game_data()->get_holding_level() + 1 + i);
				costs[commodity] += level_cost;
			}
		}

		//if increasing the holding level will also increase the province level, then the building costs more
		const int new_holding_level = site->get_game_data()->get_holding_level() + holding_level_change;
		const province *province = site->get_game_data()->get_province();
		if (new_holding_level > province->get_game_data()->get_level()) {
			const int province_level_change = new_holding_level - province->get_game_data()->get_level();
			for (int i = 0; i < province_level_change; ++i) {
				for (const auto &[commodity, base_level_cost_per_level] : defines::get()->get_province_level_commodity_costs_per_level()) {
					int64_t level_cost_per_level = base_level_cost_per_level;
					if (commodity == defines::get()->get_wealth_commodity()) {
						level_cost_per_level *= defines::get()->get_domain_income_unit_value();
					}
					costs[commodity] += level_cost_per_level * (province->get_game_data()->get_level() + 1 + i);
				}
			}
		}
	}

	if (this->get_fortification_level() > 0) {
		assert_throw(site->get_game_data()->get_holding_type() != nullptr);
		const centesimal_int fortification_level_change = site->get_game_data()->get_building_fortification_level_change(this);
		const centesimal_int total_fortification_level = site->get_game_data()->get_fortification_level() + fortification_level_change;
		for (const auto &[commodity, level_cost] : site->get_game_data()->get_holding_type()->get_fortification_level_commodity_costs()) {
			int fortification_cost = (level_cost * fortification_level_change).to_int();
			if (site->get_game_data()->is_provincial_capital() && commodity == defines::get()->get_wealth_commodity()) {
				//double the cost if fortifying the provincial capital (i.e. fortifying the province itself)
				fortification_cost *= 2;
			}
			if (total_fortification_level > site->get_game_data()->get_holding_level()) {
				//double the cost if fortifying beyond the holding level
				fortification_cost *= 2;
			}
			fortification_cost = std::max(fortification_cost, 1);
			costs[commodity] += fortification_cost;
		}
	}

	if (costs.contains(defines::get()->get_wealth_commodity()) && !this->commodity_cost_weights.empty()) {
		const commodity_map<int64_t> weighted_commodity_costs = building_type::commodity_weights_to_costs(costs.find(defines::get()->get_wealth_commodity())->second, this->commodity_cost_weights);

		for (const auto &[weighted_commodity, weighted_cost] : weighted_commodity_costs) {
			costs[weighted_commodity] += weighted_cost;
		}

		costs.erase(defines::get()->get_wealth_commodity());
	}

	const domain *domain = site->get_game_data()->get_owner();
	if (domain != nullptr) {
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

				cost = std::max(1ll, cost);
			}
		}
	}

	if (this->get_cost_factor() != nullptr) {
		for (auto &[commodity, cost] : costs) {
			cost = this->get_cost_factor()->calculate(site, decimillesimal_int(cost)).to_int();

			cost = std::max(1ll, cost);
		}
	}

	return costs;
}

QString building_type::get_commodity_costs_string_for_site(const metternich::site *site, const bool single_line) const
{
	std::string str;

	const commodity_map<int64_t> commodity_costs = this->get_commodity_costs_for_site(site);

	for (const auto &[commodity, cost] : commodity_costs) {
		if (cost == 0) {
			continue;
		}

		if (str.empty()) {
			str = "Costs:";
			if (single_line) {
				str += " ";
			} else {
				str += "\n";
			}
		} else {
			if (single_line) {
				str += ", ";
			} else {
				str += "\n";
			}
		}

		str += commodity->value_to_string(cost, commodity != defines::get()->get_wealth_commodity());
		if (commodity != defines::get()->get_wealth_commodity()) {
			str += " " + commodity->get_name();
		}
	}

	return QString::fromStdString(str);
}

std::string building_type::get_modifier_string(const site *site, const bool single_line) const
{
	std::string str;

	const std::string separator = single_line ? ", " : "\n";

	if (this->get_holding_level() != 0) {
		if (!str.empty()) {
			str += separator;
		}

		const QColor &number_color = this->get_holding_level() < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		str += std::format("Holding Level: {}", string::colored(number::to_signed_string(this->get_holding_level()), number_color));
	}

	if (this->get_fortification_level() != 0) {
		if (!str.empty()) {
			str += separator;
		}

		const QColor &number_color = this->get_fortification_level() < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		str += std::format("Fortification Level: {}", string::colored(this->get_fortification_level().to_signed_string(), number_color));
	}

	std::string site_modifier_str;
	if (this->get_modifier() != nullptr) {
		site_modifier_str += single_line ? this->get_modifier()->get_single_line_string(site) : this->get_modifier()->get_string(site);
	}

	const domain *domain = site->get_game_data()->get_owner();
	const commodity_map<int> building_commodity_bonuses = domain->get_economy()->get_building_commodity_bonuses(this);
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
			site_modifier_str += separator;
		}

		const std::string number_str = number::to_signed_string(bonus);
		const QColor &number_color = bonus < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str, number_color);

		site_modifier_str += base_string + colored_number_str;
	}

	if (!site_modifier_str.empty()) {
		if (!str.empty()) {
			str += separator;
		}

		str += site_modifier_str;
	}

	if (this->get_province_modifier() != nullptr) {
		if (!str.empty()) {
			str += separator;
		}

		const province *province = site->get_game_data()->get_province();
		str += single_line ? this->get_province_modifier()->get_single_line_string(province) : this->get_province_modifier()->get_string(province);
	}

	if (this->get_domain_modifier() != nullptr) {
		if (!str.empty()) {
			str += separator;
		}

		str += single_line ? this->get_domain_modifier()->get_single_line_string(domain) : this->get_domain_modifier()->get_string(domain);
	}

	if (this->get_weighted_domain_modifier() != nullptr) {
		if (!str.empty()) {
			str += separator;
		}

		const domain_game_data *domain_game_data = domain->get_game_data();
		const centesimal_int multiplier = centesimal_int(1) / domain_game_data->get_holding_count();
		str += single_line ? this->get_weighted_domain_modifier()->get_single_line_string(domain, multiplier.to_int())  : this->get_weighted_domain_modifier()->get_string(domain, multiplier, 0, false);
	}

	int64_t population_capacity = 0;
	if (this->get_holding_level() > 0) {
		population_capacity += this->get_population_capacity_for_province_level(site->get_game_data()->get_province()->get_game_data()->get_level(), site->get_game_data()->get_province()->get_game_data()->get_total_holding_level());
	}
	if (population_capacity > 0) {
		if (!str.empty()) {
			str += separator;
		}

		const QColor &number_color = defines::get()->get_green_text_color();
		str += std::format("Population Capacity: {}", string::colored(number::to_signed_string(population_capacity), number_color));
	}

	return str;
}

QString building_type::get_effects_string(const metternich::site *site, const bool single_line) const
{
	assert_throw(site->is_settlement());

	std::string str = this->get_modifier_string(site, single_line);

	if (this->get_effects() != nullptr) {
		if (!str.empty()) {
			str += single_line ? ", " : "\n";
		}

		const read_only_context ctx(site);
		str += single_line ? this->get_effects()->get_effects_single_line_string(site, ctx) : this->get_effects()->get_effects_string(site, ctx);
	}

	return QString::fromStdString(str);
}

}
