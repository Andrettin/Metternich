#include "script/modifier_effect/modifier_effect.h"

#include "character/character.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/modifier_effect/ai_building_desire_modifier_effect.h"
#include "script/modifier_effect/air_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/category_research_modifier_effect.h"
#include "script/modifier_effect/commodity_output_modifier_effect.h"
#include "script/modifier_effect/commodity_throughput_modifier_effect.h"
#include "script/modifier_effect/defense_modifier_effect.h"
#include "script/modifier_effect/deployment_limit_modifier_effect.h"
#include "script/modifier_effect/land_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/melee_modifier_effect.h"
#include "script/modifier_effect/naval_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/output_modifier_effect.h"
#include "script/modifier_effect/storage_capacity_modifier_effect.h"
#include "script/modifier_effect/throughput_modifier_effect.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, const country>) {
		static const std::string research_modifier_suffix = "_research_modifier";
		static const std::string throughput_modifier_suffix = "_throughput_modifier";

		if (key == "air_morale_resistance") {
			return std::make_unique<air_morale_resistance_modifier_effect>(value);
		} else if (key == "deployment_limit") {
			return std::make_unique<deployment_limit_modifier_effect>(value);
		} else if (key == "land_morale_resistance") {
			return std::make_unique<land_morale_resistance_modifier_effect>(value);
		} else if (key == "naval_morale_resistance") {
			return std::make_unique<naval_morale_resistance_modifier_effect>(value);
		} else if (key == "storage_capacity") {
			return std::make_unique<storage_capacity_modifier_effect>(value);
		} else if (key == "throughput_modifier") {
			return std::make_unique<throughput_modifier_effect<scope_type>>(value);
		} else if (key.ends_with(throughput_modifier_suffix) && commodity::try_get(key.substr(0, key.size() - throughput_modifier_suffix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(0, key.size() - throughput_modifier_suffix.size()));
			return std::make_unique<commodity_throughput_modifier_effect<scope_type>>(commodity, value);
		} else if (key.ends_with(research_modifier_suffix) && enum_converter<technology_category>::has_value(key.substr(0, key.size() - research_modifier_suffix.size()))) {
			const technology_category category = enum_converter<technology_category>::to_enum(key.substr(0, key.size() - research_modifier_suffix.size()));
			return std::make_unique<category_research_modifier_effect<scope_type>>(category, value);
		}
	} else if constexpr (std::is_same_v<scope_type, military_unit>) {
		if (key == "defense") {
			return std::make_unique<defense_modifier_effect>(value);
		} else if (key == "melee") {
			return std::make_unique<melee_modifier_effect>(value);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, const country> || std::is_same_v<scope_type, const province>) {
		static const std::string output_modifier_suffix = "_output_modifier";

		if (key == "output_modifier") {
			return std::make_unique<output_modifier_effect<scope_type>>(value);
		} else if (key.ends_with(output_modifier_suffix) && commodity::try_get(key.substr(0, key.size() - output_modifier_suffix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(0, key.size() - output_modifier_suffix.size()));
			return std::make_unique<commodity_output_modifier_effect<scope_type>>(commodity, value);
		}
	}

	throw std::runtime_error("Invalid property modifier effect: \"" + key + "\".");
}


template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	std::unique_ptr<modifier_effect> modifier_effect;

	if constexpr (std::is_same_v<scope_type, const country>) {
		if (tag == "ai_building_desire") {
			modifier_effect = std::make_unique<ai_building_desire_modifier_effect>();
		}
	}

	if (modifier_effect == nullptr) {
		throw std::runtime_error("Invalid scope modifier effect: \"" + tag + "\".");
	}

	database::process_gsml_data(modifier_effect, scope);

	return modifier_effect;
}

template <typename scope_type>
void modifier_effect<scope_type>::process_gsml_property(const gsml_property &property)
{
	throw std::runtime_error(std::format("Invalid property for \"{}\" effect: \"{}\".", this->get_identifier(), property.get_key()));
}

template <typename scope_type>
void modifier_effect<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error(std::format("Invalid scope for \"{}\" effect: \"{}\".", this->get_identifier(), scope.get_tag()));
}

template class modifier_effect<const character>;
template class modifier_effect<const country>;
template class modifier_effect<military_unit>;
template class modifier_effect<const province>;

}
