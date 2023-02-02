#include "metternich.h"

#include "script/condition/condition.h"

#include "country/country_game_data.h"
#include "database/database.h"
#include "database/gsml_operator.h"
#include "database/named_data_entry.h"
#include "map/province_game_data.h"
#include "population/population_unit.h"
#include "script/condition/age_condition.h"
#include "script/condition/and_condition.h"
#include "script/condition/character_type_condition.h"
#include "script/condition/coastal_condition.h"
#include "script/condition/core_condition.h"
#include "script/condition/country_type_condition.h"
#include "script/condition/has_country_office_condition.h"
#include "script/condition/has_province_office_condition.h"
#include "script/condition/is_ruler_condition.h"
#include "script/condition/location_condition.h"
#include "script/condition/not_condition.h"
#include "script/condition/or_condition.h"
#include "script/condition/piety_condition.h"
#include "script/condition/prestige_condition.h"
#include "script/condition/religion_condition.h"
#include "script/condition/religious_group_condition.h"
#include "script/condition/trait_condition.h"
#include "script/condition/war_condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<const condition<scope_type>> condition<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator condition_operator = property.get_operator();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, character>) {
		if (key == "age") {
			return std::make_unique<age_condition>(value, condition_operator);
		} else if (key == "character_type") {
			return std::make_unique<character_type_condition>(value, condition_operator);
		} else if (key == "has_country_office") {
			return std::make_unique<has_country_office_condition>(value, condition_operator);
		} else if (key == "has_province_office") {
			return std::make_unique<has_province_office_condition>(value, condition_operator);
		} else if (key == "is_ruler") {
			return std::make_unique<is_ruler_condition>(value, condition_operator);
		} else if (key == "piety") {
			return std::make_unique<piety_condition>(value, condition_operator);
		} else if (key == "prestige") {
			return std::make_unique<prestige_condition>(value, condition_operator);
		} else if (key == "trait") {
			return std::make_unique<trait_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, country>) {
		if (key == "country_type") {
			return std::make_unique<country_type_condition>(value, condition_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, province>) {
		if (key == "core") {
			return std::make_unique<core_condition>(value, condition_operator);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, character> || std::is_same_v<scope_type, country>) {
		if (key == "war") {
			return std::make_unique<war_condition<scope_type>>(value, condition_operator);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, country> || std::is_same_v<scope_type, province>) {
		if (key == "coastal") {
			return std::make_unique<coastal_condition<scope_type>>(value, condition_operator);
		}
	}

	if (key == "religion") {
		return std::make_unique<religion_condition<scope_type>>(value, condition_operator);
	} else if (key == "religious_group") {
		return std::make_unique<religious_group_condition<scope_type>>(value, condition_operator);
	}

	throw std::runtime_error("Invalid condition property: \"" + key + "\".");
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

	if constexpr (std::is_same_v<scope_type, population_unit>) {
		if (tag == "location") {
			condition = std::make_unique<location_condition<scope_type>>(condition_operator);
		}
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
