#include "metternich.h"

#include "script/condition/condition.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "database/gsml_operator.h"
#include "database/named_data_entry.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population_unit.h"
#include "script/condition/adjacent_terrain_condition.h"
#include "script/condition/advisor_condition.h"
#include "script/condition/advisor_category_condition.h"
#include "script/condition/advisor_type_condition.h"
#include "script/condition/age_condition.h"
#include "script/condition/anarchy_condition.h"
#include "script/condition/and_condition.h"
#include "script/condition/any_global_population_unit_condition.h"
#include "script/condition/any_known_country_condition.h"
#include "script/condition/any_neighbor_country_condition.h"
#include "script/condition/any_population_unit_condition.h"
#include "script/condition/any_settlement_condition.h"
#include "script/condition/artillery_condition.h"
#include "script/condition/attacking_commander_condition.h"
#include "script/condition/available_food_condition.h"
#include "script/condition/available_health_condition.h"
#include "script/condition/birth_year_condition.h"
#include "script/condition/can_gain_building_class_condition.h"
#include "script/condition/can_have_trait_condition.h"
#include "script/condition/capital_condition.h"
#include "script/condition/cavalry_condition.h"
#include "script/condition/coastal_condition.h"
#include "script/condition/commodity_condition.h"
#include "script/condition/consciousness_condition.h"
#include "script/condition/core_condition.h"
#include "script/condition/country_condition.h"
#include "script/condition/country_exists_condition.h"
#include "script/condition/country_scope_condition.h"
#include "script/condition/country_type_condition.h"
#include "script/condition/cultural_group_condition.h"
#include "script/condition/culture_condition.h"
#include "script/condition/discovered_province_condition.h"
#include "script/condition/discovered_region_condition.h"
#include "script/condition/event_condition.h"
#include "script/condition/game_rule_condition.h"
#include "script/condition/gender_condition.h"
#include "script/condition/government_type_condition.h"
#include "script/condition/has_building_condition.h"
#include "script/condition/has_building_class_condition.h"
#include "script/condition/has_flag_condition.h"
#include "script/condition/has_population_culture_condition.h"
#include "script/condition/has_population_ideology_condition.h"
#include "script/condition/has_population_religion_condition.h"
#include "script/condition/has_population_type_condition.h"
#include "script/condition/has_resource_condition.h"
#include "script/condition/has_route_condition.h"
#include "script/condition/has_terrain_condition.h"
#include "script/condition/health_condition.h"
#include "script/condition/ideology_condition.h"
#include "script/condition/improvement_condition.h"
#include "script/condition/independent_condition.h"
#include "script/condition/infantry_condition.h"
#include "script/condition/inflation_condition.h"
#include "script/condition/is_advisor_condition.h"
#include "script/condition/is_military_unit_category_available_condition.h"
#include "script/condition/is_ruler_condition.h"
#include "script/condition/law_condition.h"
#include "script/condition/militancy_condition.h"
#include "script/condition/military_unit_category_condition.h"
#include "script/condition/military_unit_domain_condition.h"
#include "script/condition/military_unit_type_condition.h"
#include "script/condition/near_water_condition.h"
#include "script/condition/not_condition.h"
#include "script/condition/or_condition.h"
#include "script/condition/owns_province_condition.h"
#include "script/condition/owns_site_condition.h"
#include "script/condition/population_scaled_commodity_condition.h"
#include "script/condition/population_type_condition.h"
#include "script/condition/population_unit_count_condition.h"
#include "script/condition/produces_commodity_condition.h"
#include "script/condition/promotion_condition.h"
#include "script/condition/province_condition.h"
#include "script/condition/province_count_condition.h"
#include "script/condition/provincial_capital_condition.h"
#include "script/condition/religion_condition.h"
#include "script/condition/religious_group_condition.h"
#include "script/condition/resource_condition.h"
#include "script/condition/river_condition.h"
#include "script/condition/root_character_condition.h"
#include "script/condition/ruler_condition.h"
#include "script/condition/ruler_scope_condition.h"
#include "script/condition/saved_scope_condition.h"
#include "script/condition/scripted_condition_condition.h"
#include "script/condition/scripted_modifier_condition.h"
#include "script/condition/settlement_condition.h"
#include "script/condition/settlement_type_condition.h"
#include "script/condition/source_character_condition.h"
#include "script/condition/source_site_condition.h"
#include "script/condition/source_site_scope_condition.h"
#include "script/condition/subject_type_condition.h"
#include "script/condition/technology_condition.h"
#include "script/condition/terrain_condition.h"
#include "script/condition/tooltip_condition.h"
#include "script/condition/tradition_condition.h"
#include "script/condition/trait_condition.h"
#include "script/condition/war_condition.h"
#include "script/condition/wealth_condition.h"
#include "script/condition/wealth_inflated_condition.h"
#include "script/condition/year_condition.h"
#include "unit/military_unit.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<const condition<scope_type>> condition<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator condition_operator = property.get_operator();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, character>) {
		if (key == "advisor_category") {
			return std::make_unique<advisor_category_condition>(value, condition_operator);
		} else if (key == "advisor_type") {
			return std::make_unique<advisor_type_condition>(value, condition_operator);
		} else if (key == "age") {
			return std::make_unique<age_condition>(value, condition_operator);
		} else if (key == "birth_year") {
			return std::make_unique<birth_year_condition>(value, condition_operator);
		} else if (key == "can_have_trait") {
			return std::make_unique<can_have_trait_condition>(value, condition_operator);
		} else if (key == "gender") {
			return std::make_unique<gender_condition>(value, condition_operator);
		} else if (key == "is_advisor") {
			return std::make_unique<is_advisor_condition>(value, condition_operator);
		} else if (key == "is_ruler") {
			return std::make_unique<is_ruler_condition>(value, condition_operator);
		} else if (key == "trait") {
			return std::make_unique<trait_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, country>) {
		static const std::string population_scaled_commodity_prefix = "population_scaled_";

		if (key == "advisor") {
			return std::make_unique<advisor_condition>(value, condition_operator);
		} else if (key == "anarchy") {
			return std::make_unique<anarchy_condition>(value, condition_operator);
		} else if (key == "available_food") {
			return std::make_unique<available_food_condition>(value, condition_operator);
		} else if (key == "country_type") {
			return std::make_unique<country_type_condition>(value, condition_operator);
		} else if (key == "discovered_province") {
			return std::make_unique<discovered_province_condition>(value, condition_operator);
		} else if (key == "discovered_region") {
			return std::make_unique<discovered_region_condition>(value, condition_operator);
		} else if (key == "government_type") {
			return std::make_unique<government_type_condition>(value, condition_operator);
		} else if (key == "has_flag") {
			return std::make_unique<has_flag_condition>(value, condition_operator);
		} else if (key == "inflation") {
			return std::make_unique<inflation_condition>(value, condition_operator);
		} else if (key == "independent") {
			return std::make_unique<independent_condition>(value, condition_operator);
		} else if (key == "is_military_unit_category_available") {
			return std::make_unique<is_military_unit_category_available_condition>(value, condition_operator);
		} else if (key == "owns_province") {
			return std::make_unique<owns_province_condition>(value, condition_operator);
		} else if (key == "owns_site") {
			return std::make_unique<owns_site_condition>(value, condition_operator);
		} else if (key == "province_count") {
			return std::make_unique<province_count_condition>(value, condition_operator);
		} else if (key == "ruler") {
			return std::make_unique<ruler_condition>(value, condition_operator);
		} else if (key == "subject_type") {
			return std::make_unique<subject_type_condition>(value, condition_operator);
		} else if (key == "wealth") {
			return std::make_unique<wealth_condition<scope_type>>(value, condition_operator);
		} else if (key == "wealth_inflated") {
			return std::make_unique<wealth_inflated_condition>(value, condition_operator);
		} else if (commodity::try_get(key) != nullptr && string::is_number(value)) {
			return std::make_unique<commodity_condition>(commodity::get(key), value, condition_operator);
		} else if (key.starts_with(population_scaled_commodity_prefix) && commodity::try_get(key.substr(population_scaled_commodity_prefix.size(), key.size() - population_scaled_commodity_prefix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(population_scaled_commodity_prefix.size(), key.size() - population_scaled_commodity_prefix.size()));
			return std::make_unique<population_scaled_commodity_condition>(commodity, value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, military_unit>) {
		if (key == "artillery") {
			return std::make_unique<artillery_condition>(value, condition_operator);
		} else if (key == "cavalry") {
			return std::make_unique<cavalry_condition>(value, condition_operator);
		} else if (key == "infantry") {
			return std::make_unique<infantry_condition>(value, condition_operator);
		} else if (key == "military_unit_category") {
			return std::make_unique<military_unit_category_condition>(value, condition_operator);
		} else if (key == "military_unit_domain") {
			return std::make_unique<military_unit_domain_condition>(value, condition_operator);
		} else if (key == "military_unit_type") {
			return std::make_unique<military_unit_type_condition>(value, condition_operator);
		} else if (key == "promotion") {
			return std::make_unique<promotion_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, population_unit>) {
		if (key == "ideology") {
			return std::make_unique<ideology_condition>(value, condition_operator);
		} else if (key == "population_type") {
			return std::make_unique<population_type_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, province>) {
		if (key == "core") {
			return std::make_unique<core_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, site>) {
		if (key == "adjacent_terrain") {
			return std::make_unique<adjacent_terrain_condition>(value, condition_operator);
		} else if (key == "can_gain_building_class") {
			return std::make_unique<can_gain_building_class_condition>(value, condition_operator);
		} else if (key == "has_route") {
			return std::make_unique<has_route_condition<site>>(value, condition_operator);
		} else if (key == "improvement") {
			return std::make_unique<improvement_condition>(value, condition_operator);
		} else if (key == "resource") {
			return std::make_unique<resource_condition>(value, condition_operator);
		} else if (key == "river") {
			return std::make_unique<river_condition>(value, condition_operator);
		} else if (key == "settlement_type") {
			return std::make_unique<settlement_type_condition>(value, condition_operator);
		} else if (key == "terrain") {
			return std::make_unique<terrain_condition>(value, condition_operator);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, country>) {
		if (key == "war") {
			return std::make_unique<war_condition<scope_type>>(value, condition_operator);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, province> || std::is_same_v<scope_type, site>) {
		if (key == "scripted_modifier") {
			return std::make_unique<scripted_modifier_condition<scope_type>>(value, condition_operator);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, province>) {
		if (key == "has_resource") {
			return std::make_unique<has_resource_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_terrain") {
			return std::make_unique<has_terrain_condition<scope_type>>(value, condition_operator);
		} else if (key == "produces_commodity") {
			return std::make_unique<produces_commodity_condition<scope_type>>(value, condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, province> || std::is_same_v<scope_type, site>) {
		if (key == "capital") {
			return std::make_unique<capital_condition<scope_type>>(value, condition_operator);
		} else if (key == "coastal") {
			return std::make_unique<coastal_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_building") {
			return std::make_unique<has_building_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_building_class") {
			return std::make_unique<has_building_class_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_population_culture") {
			return std::make_unique<has_population_culture_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_population_ideology") {
			return std::make_unique<has_population_ideology_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_population_religion") {
			return std::make_unique<has_population_religion_condition<scope_type>>(value, condition_operator);
		} else if (key == "has_population_type") {
			return std::make_unique<has_population_type_condition<scope_type>>(value, condition_operator);
		} else if (key == "population_unit_count") {
			return std::make_unique<population_unit_count_condition<scope_type>>(value, condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, site>) {
		if (key == "available_health") {
			return std::make_unique<available_health_condition<scope_type>>(value, condition_operator);
		} else if (key == "health") {
			return std::make_unique<health_condition<scope_type>>(value, condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, province> || std::is_same_v<scope_type, site>) {
		if (key == "near_water") {
			return std::make_unique<near_water_condition<scope_type>>(value, condition_operator);
		} else if (key == "provincial_capital") {
			return std::make_unique<provincial_capital_condition<scope_type>>(value, condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, population_unit> || std::is_same_v<scope_type, site> || std::is_same_v<scope_type, province>) {
		if (key == "consciousness") {
			return std::make_unique<consciousness_condition<scope_type>>(value, condition_operator);
		} else if (key == "militancy") {
			return std::make_unique<militancy_condition<scope_type>>(value, condition_operator);
		}
	}

	if (key == "country") {
		return std::make_unique<country_condition<scope_type>>(value, condition_operator);
	} else if (key == "country_exists") {
		return std::make_unique<country_exists_condition<scope_type>>(value, condition_operator);
	} else if (key == "cultural_group") {
		return std::make_unique<cultural_group_condition<scope_type>>(value, condition_operator);
	} else if (key == "culture") {
		return std::make_unique<culture_condition<scope_type>>(value, condition_operator);
	} else if (key == "event") {
		return std::make_unique<event_condition<scope_type>>(value, condition_operator);
	} else if (key == "game_rule") {
		return std::make_unique<game_rule_condition<scope_type>>(value, condition_operator);
	} else if (key == "law") {
		return std::make_unique<law_condition<scope_type>>(value, condition_operator);
	} else if (key == "religion") {
		return std::make_unique<religion_condition<scope_type>>(value, condition_operator);
	} else if (key == "religious_group") {
		return std::make_unique<religious_group_condition<scope_type>>(value, condition_operator);
	} else if (key == "root_character") {
		return std::make_unique<root_character_condition<scope_type>>(value, condition_operator);
	} else if (key == "scripted_condition") {
		return std::make_unique<scripted_condition_condition<scope_type>>(value, condition_operator);
	} else if (key == "source_character") {
		return std::make_unique<source_character_condition<scope_type>>(value, condition_operator);
	} else if (key == "source_site") {
		return std::make_unique<source_site_condition<scope_type>>(value, condition_operator);
	} else if (key == "technology") {
		return std::make_unique<technology_condition<scope_type>>(value, condition_operator);
	} else if (key == "tradition") {
		return std::make_unique<tradition_condition<scope_type>>(value, condition_operator);
	} else if (key == "year") {
		return std::make_unique<year_condition<scope_type>>(value, condition_operator);
	}

	throw std::runtime_error(std::format("Invalid condition property: \"{}\".", key));
}

template <typename scope_type>
std::unique_ptr<const condition<scope_type>> condition<scope_type>::from_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const gsml_operator condition_operator = scope.get_operator();
	std::unique_ptr<condition<scope_type>> condition;

	if (tag == "and") {
		condition = std::make_unique<and_condition<scope_type>>(condition_operator);
	} else if (tag == "or") {
		condition = std::make_unique<or_condition<scope_type>>(condition_operator);
	} else if (tag == "not") {
		condition = std::make_unique<not_condition<scope_type>>(condition_operator);
	}

	if constexpr (std::is_same_v<scope_type, country>) {
		if (tag == "any_known_country") {
			condition = std::make_unique<any_known_country_condition>(condition_operator);
		} else if (tag == "any_neighbor_country") {
			condition = std::make_unique<any_neighbor_country_condition>(condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, population_unit>) {
		if (tag == "settlement") {
			condition = std::make_unique<settlement_condition>(condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, province>) {
		if (tag == "any_settlement") {
			condition = std::make_unique<any_settlement_condition<scope_type>>(condition_operator);
		} else if (tag == "ruler") {
			condition = std::make_unique<ruler_scope_condition<scope_type>>(condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, province> || std::is_same_v<scope_type, site>) {
		if (tag == "any_population_unit") {
			condition = std::make_unique<any_population_unit_condition<scope_type>>(condition_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, population_unit> || std::is_same_v<scope_type, site>) {
		if (tag == "province") {
			condition = std::make_unique<province_condition<scope_type>>(condition_operator);
		}
	}

	if (tag == "any_global_population_unit") {
		condition = std::make_unique<any_global_population_unit_condition<scope_type>>(condition_operator);
	} else if (tag == "attacking_commander") {
		condition = std::make_unique<attacking_commander_condition<scope_type>>(condition_operator);
	} else if (tag == "country") {
		condition = std::make_unique<country_scope_condition<scope_type>>(condition_operator);
	} else if (tag == "saved_character_scope") {
		condition = std::make_unique<saved_scope_condition<scope_type, character>>(condition_operator);
	} else if (tag == "saved_country_scope") {
		condition = std::make_unique<saved_scope_condition<scope_type, country>>(condition_operator);
	} else if (tag == "saved_province_scope") {
		condition = std::make_unique<saved_scope_condition<scope_type, province>>(condition_operator);
	} else if (tag == "saved_site_scope") {
		condition = std::make_unique<saved_scope_condition<scope_type, site>>(condition_operator);
	} else if (tag == "source_site") {
		condition = std::make_unique<source_site_scope_condition<scope_type>>(condition_operator);
	} else if (tag == "tooltip") {
		condition = std::make_unique<tooltip_condition<scope_type>>(condition_operator);
	}

	if (condition == nullptr) {
		throw std::runtime_error("Invalid condition scope: \"" + tag + "\".");
	}

	database::process_gsml_data(condition, scope);

	return condition;
}

template <typename scope_type>
std::string condition<scope_type>::get_object_highlighted_name(const named_data_entry *object, const std::string &name_string)
{
	if (!name_string.empty()) {
		return string::highlight(name_string);
	} else {
		return string::highlight(object->get_name());
	}
}

template <typename scope_type>
const country *condition<scope_type>::get_scope_country(const scope_type *scope)
{
	if constexpr (std::is_same_v<scope_type, character>) {
		return scope->get_game_data()->get_country();
	} else if constexpr (std::is_same_v<scope_type, country>) {
		return scope;
	} else if constexpr (std::is_same_v<scope_type, military_unit> || std::is_same_v<scope_type, population_unit>) {
		return scope->get_country();
	} else if constexpr (std::is_same_v<scope_type, province> || std::is_same_v<scope_type, site>) {
		return scope->get_game_data()->get_owner();
	}
}

template <typename scope_type>
const province *condition<scope_type>::get_scope_province(const scope_type *scope)
{
	if constexpr (std::is_same_v<scope_type, population_unit>) {
		return scope->get_province();
	} else if constexpr (std::is_same_v<scope_type, province>) {
		return scope;
	} else if constexpr (std::is_same_v<scope_type, site>) {
		return scope->get_game_data()->get_province();
	}
}

template <typename scope_type>
condition<scope_type>::condition(const gsml_operator condition_operator) : condition_operator(condition_operator)
{
}

template <typename scope_type>
void condition<scope_type>::process_gsml_property(const gsml_property &property)
{
	throw std::runtime_error("Invalid \"" + this->get_class_identifier() + "\" condition property: \"" + property.get_key() + "\".");
}

template <typename scope_type>
void condition<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid \"" + this->get_class_identifier() + "\" condition scope: \"" + scope.get_tag() + "\".");
}


template <typename scope_type>
bool condition<scope_type>::check(const scope_type *scope, const read_only_context &ctx) const
{
	switch (this->condition_operator) {
		case gsml_operator::assignment:
			return this->check_assignment(scope, ctx);
		case gsml_operator::equality:
			return this->check_equality(scope);
		case gsml_operator::inequality:
			return this->check_inequality(scope);
		case gsml_operator::less_than:
			return this->check_less_than(scope);
		case gsml_operator::less_than_or_equality:
			return this->check_less_than_or_equality(scope);
		case gsml_operator::greater_than:
			return this->check_greater_than(scope);
		case gsml_operator::greater_than_or_equality:
			return this->check_greater_than_or_equality(scope);
		default:
			throw std::runtime_error("Invalid condition operator: \"" + std::to_string(static_cast<int>(this->condition_operator)) + "\".");
	}
}

template <typename scope_type>
std::string condition<scope_type>::get_string(const size_t indent) const
{
	switch (this->condition_operator) {
		case gsml_operator::assignment:
			return this->get_assignment_string(indent);
		case gsml_operator::equality:
			return this->get_equality_string();
		case gsml_operator::inequality:
			return this->get_inequality_string();
		case gsml_operator::less_than:
			return this->get_less_than_string();
		case gsml_operator::less_than_or_equality:
			return this->get_less_than_or_equality_string();
		case gsml_operator::greater_than:
			return this->get_greater_than_string();
		case gsml_operator::greater_than_or_equality:
			return this->get_greater_than_or_equality_string();
		default:
			throw std::runtime_error("Invalid condition operator: \"" + std::to_string(static_cast<int>(this->condition_operator)) + "\".");
	}
}

}
