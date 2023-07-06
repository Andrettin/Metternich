#include "metternich.h"

#include "script/modifier_effect/modifier_effect.h"

#include "character/character.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/modifier_effect/advisor_cost_modifier_effect.h"
#include "script/modifier_effect/ai_building_desire_modifier_effect.h"
#include "script/modifier_effect/air_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/artillery_cost_modifier_effect.h"
#include "script/modifier_effect/building_capacity_modifier_effect.h"
#include "script/modifier_effect/category_research_modifier_effect.h"
#include "script/modifier_effect/cavalry_cost_modifier_effect.h"
#include "script/modifier_effect/commodity_bonus_for_tile_threshold_modifier_effect.h"
#include "script/modifier_effect/commodity_per_improved_resource_modifier_effect.h"
#include "script/modifier_effect/commodity_output_modifier_effect.h"
#include "script/modifier_effect/commodity_throughput_modifier_effect.h"
#include "script/modifier_effect/defense_modifier_effect.h"
#include "script/modifier_effect/deployment_limit_modifier_effect.h"
#include "script/modifier_effect/diplomatic_penalty_for_expansion_modifier_effect.h"
#include "script/modifier_effect/free_consulate_modifier_effect.h"
#include "script/modifier_effect/gain_technologies_known_by_others_modifier_effect.h"
#include "script/modifier_effect/infantry_cost_modifier_effect.h"
#include "script/modifier_effect/land_damage_modifier_effect.h"
#include "script/modifier_effect/land_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/land_recovery_modifier_effect.h"
#include "script/modifier_effect/melee_modifier_effect.h"
#include "script/modifier_effect/naval_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/output_modifier_effect.h"
#include "script/modifier_effect/storage_capacity_modifier_effect.h"
#include "script/modifier_effect/throughput_modifier_effect.h"
#include "script/modifier_effect/unit_upgrade_cost_modifier_effect.h"
#include "util/number_util.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, const country>) {
		static const std::string building_capacity_modifier_suffix = "_capacity";
		static const std::string research_modifier_suffix = "_research_modifier";
		static const std::string throughput_modifier_suffix = "_throughput_modifier";

		if (key == "advisor_cost_modifier") {
			return std::make_unique<advisor_cost_modifier_effect>(value);
		} else if (key == "air_morale_resistance") {
			return std::make_unique<air_morale_resistance_modifier_effect>(value);
		} else if (key == "artillery_cost_modifier") {
			return std::make_unique<artillery_cost_modifier_effect>(value);
		} else if (key == "cavalry_cost_modifier") {
			return std::make_unique<cavalry_cost_modifier_effect>(value);
		} else if (key == "deployment_limit") {
			return std::make_unique<deployment_limit_modifier_effect>(value);
		} else if (key == "diplomatic_penalty_for_expansion_modifier") {
			return std::make_unique<diplomatic_penalty_for_expansion_modifier_effect>(value);
		} else if (key == "free_consulate") {
			return std::make_unique<free_consulate_modifier_effect>(value);
		} else if (key == "gain_technologies_known_by_others") {
			return std::make_unique<gain_technologies_known_by_others_modifier_effect>(value);
		} else if (key == "infantry_cost_modifier") {
			return std::make_unique<infantry_cost_modifier_effect>(value);
		} else if (key == "land_damage_modifier") {
			return std::make_unique<land_damage_modifier_effect>(value);
		} else if (key == "land_morale_resistance") {
			return std::make_unique<land_morale_resistance_modifier_effect>(value);
		} else if (key == "land_recovery_modifier") {
			return std::make_unique<land_recovery_modifier_effect>(value);
		} else if (key == "naval_morale_resistance") {
			return std::make_unique<naval_morale_resistance_modifier_effect>(value);
		} else if (key == "storage_capacity") {
			return std::make_unique<storage_capacity_modifier_effect>(value);
		} else if (key == "throughput_modifier") {
			return std::make_unique<throughput_modifier_effect<scope_type>>(value);
		} else if (key == "unit_upgrade_cost_modifier") {
			return std::make_unique<unit_upgrade_cost_modifier_effect>(value);
		} else if (key.ends_with(building_capacity_modifier_suffix) && building_slot_type::try_get(key.substr(0, key.size() - building_capacity_modifier_suffix.size())) != nullptr) {
			const building_slot_type *building_slot_type = building_slot_type::get(key.substr(0, key.size() - building_capacity_modifier_suffix.size()));
			return std::make_unique<building_capacity_modifier_effect>(building_slot_type, value);
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
		static const std::string commodity_bonus_for_tile_threshold_suffix = "_bonus_for_tile_threshold";
		static const std::string commodity_per_improved_resource_infix = "_per_improved_";
		static const std::string output_modifier_suffix = "_output_modifier";

		if (key == "output_modifier") {
			return std::make_unique<output_modifier_effect<scope_type>>(value);
		} else if (key.ends_with(output_modifier_suffix) && commodity::try_get(key.substr(0, key.size() - output_modifier_suffix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(0, key.size() - output_modifier_suffix.size()));
			return std::make_unique<commodity_output_modifier_effect<scope_type>>(commodity, value);
		} else if (key.ends_with(commodity_bonus_for_tile_threshold_suffix)) {
			const size_t commodity_identifier_size = key.size() - commodity_bonus_for_tile_threshold_suffix.size();
			const commodity *commodity = commodity::get(key.substr(0, commodity_identifier_size));

			return std::make_unique<commodity_bonus_for_tile_threshold_modifier_effect<scope_type>>(commodity, value);
		} else if (key.find(commodity_per_improved_resource_infix) != std::string::npos && !key.starts_with(commodity_per_improved_resource_infix) && !key.ends_with(commodity_per_improved_resource_infix)) {
			const size_t infix_pos = key.find(commodity_per_improved_resource_infix);
			const commodity *commodity = commodity::get(key.substr(0, infix_pos));

			const size_t resource_identifier_pos = infix_pos + commodity_per_improved_resource_infix.size();
			const resource *resource = resource::get(key.substr(resource_identifier_pos, key.size() - resource_identifier_pos));

			return std::make_unique<commodity_per_improved_resource_modifier_effect<scope_type>>(commodity, resource, value);
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

template <typename scope_type>
centesimal_int modifier_effect<scope_type>::get_multiplied_value(const centesimal_int &multiplier) const
{
	return this->value * multiplier;
}

template <typename scope_type>
std::string modifier_effect<scope_type>::get_string(const centesimal_int &multiplier, const bool ignore_decimals) const
{
	const centesimal_int value = this->get_multiplied_value(multiplier);
	const std::string number_str = ignore_decimals ? number::to_signed_string(value.to_int()) : value.to_signed_string();
	const QColor &number_color = this->is_negative() ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
	const std::string colored_number_str = string::colored(number_str + (this->is_percent() ? "%" : ""), number_color);

	return std::format("{}: {}", this->get_base_string(), colored_number_str);
}

template class modifier_effect<const character>;
template class modifier_effect<const country>;
template class modifier_effect<military_unit>;
template class modifier_effect<const province>;

}
