#include "metternich.h"

#include "script/effect/effect.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/any_character_effect.h"
#include "script/effect/any_character_or_vassal_effect.h"
#include "script/effect/any_population_unit_effect.h"
#include "script/effect/any_vassal_character_effect.h"
#include "script/effect/battle_effect.h"
#include "script/effect/commodity_effect.h"
#include "script/effect/commodity_percent_effect.h"
#include "script/effect/consciousness_effect.h"
#include "script/effect/country_effect.h"
#include "script/effect/delayed_effect.h"
#include "script/effect/event_effect.h"
#include "script/effect/gain_item_effect.h"
#include "script/effect/gain_spell_scroll_effect.h"
#include "script/effect/hidden_effect.h"
#include "script/effect/if_effect.h"
#include "script/effect/militancy_effect.h"
#include "script/effect/opinion_modifiers_effect.h"
#include "script/effect/piety_effect.h"
#include "script/effect/prestige_effect.h"
#include "script/effect/random_list_effect.h"
#include "script/effect/save_scope_as_effect.h"
#include "script/effect/scripted_effect_effect.h"
#include "script/effect/scripted_modifiers_effect.h"
#include "script/effect/source_site_effect.h"
#include "script/effect/tooltip_effect.h"
#include "script/effect/traits_effect.h"
#include "script/effect/wealth_effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<effect<scope_type>> effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator effect_operator = property.get_operator();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, const character>) {
		if (key == "scripted_modifiers") {
			assert_throw(effect_operator == gsml_operator::subtraction);
			return std::make_unique<scripted_modifiers_effect<const character>>(value, effect_operator);
		} else if (key == "traits") {
			return std::make_unique<traits_effect>(value, effect_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		static const std::string percent_suffix = "_percent";

		if (key == "gain_item") {
			return std::make_unique<gain_item_effect>(value, effect_operator);
		} else if (key == "gain_spell_scroll") {
			return std::make_unique<gain_spell_scroll_effect>(value, effect_operator);
		} else if (commodity::try_get(key) != nullptr) {
			return std::make_unique<commodity_effect>(commodity::get(key), value, effect_operator);
		} else if (key.ends_with(percent_suffix) && commodity::try_get(key.substr(0, key.size() - percent_suffix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(0, key.size() - percent_suffix.size()));
			return std::make_unique<commodity_percent_effect>(commodity, value, effect_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, population_unit>) {
		if (key == "consciousness") {
			return std::make_unique<consciousness_effect>(value, effect_operator);
		} else if (key == "militancy") {
			return std::make_unique<militancy_effect>(value, effect_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, const character> || std::is_same_v<scope_type, const country>) {
		if (key == "event") {
			return std::make_unique<event_effect<scope_type>>(value, effect_operator);
		} else if (key == "piety") {
			return std::make_unique<piety_effect<scope_type>>(value, effect_operator);
		} else if (key == "prestige") {
			return std::make_unique<prestige_effect<scope_type>>(value, effect_operator);
		} else if (key == "wealth") {
			return std::make_unique<wealth_effect<scope_type>>(value, effect_operator);
		}
	}

	if (key == "save_scope_as") {
		return std::make_unique<save_scope_as_effect<scope_type>>(value, effect_operator);
	} else if (key == "scripted_effect") {
		return std::make_unique<scripted_effect_effect<scope_type>>(value, effect_operator);
	} else if (key == "tooltip") {
		return std::make_unique<tooltip_effect<scope_type>>(value, effect_operator);
	}

	throw std::runtime_error("Invalid property effect: \"" + key + "\".");
}

template <typename scope_type>
std::unique_ptr<effect<scope_type>> effect<scope_type>::from_gsml_scope(const gsml_data &scope)
{
	const std::string &effect_identifier = scope.get_tag();
	const gsml_operator effect_operator = scope.get_operator();
	std::unique_ptr<effect> effect;

	if constexpr (std::is_same_v<scope_type, const character>) {
		if (effect_identifier == "opinion_modifiers") {
			effect = std::make_unique<opinion_modifiers_effect<scope_type>>(effect_operator);
		} else if (effect_identifier == "scripted_modifiers") {
			effect = std::make_unique<scripted_modifiers_effect<scope_type>>(effect_operator);
		}
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		if (effect_identifier == "battle") {
			effect = std::make_unique<battle_effect<scope_type>>(effect_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, const character> || std::is_same_v<scope_type, const country>) {
		if (effect_identifier == "any_character") {
			effect = std::make_unique<any_character_effect<scope_type>>(effect_operator);
		} else if (effect_identifier == "any_character_or_vassal") {
			effect = std::make_unique<any_character_or_vassal_effect<scope_type>>(effect_operator);
		} else if (effect_identifier == "any_vassal_character") {
			effect = std::make_unique<any_vassal_character_effect<scope_type>>(effect_operator);
		} else if (effect_identifier == "delayed") {
			effect = std::make_unique<delayed_effect<scope_type>>(effect_operator);
		}
	}

	if constexpr (std::is_same_v<scope_type, const country> || std::is_same_v<scope_type, const province>) {
		if (effect_identifier == "any_population_unit") {
			effect = std::make_unique<any_population_unit_effect<scope_type>>(effect_operator);
		}
	}

	if (effect_identifier == "country") {
		effect = std::make_unique<country_effect<scope_type>>(effect_operator);
	} else if (effect_identifier == "hidden") {
		effect = std::make_unique<hidden_effect<scope_type>>(effect_operator);
	} else if (effect_identifier == "if") {
		effect = std::make_unique<if_effect<scope_type>>(effect_operator);
	} else if (effect_identifier == "random_list") {
		effect = std::make_unique<random_list_effect<scope_type>>(effect_operator);
	} else if (effect_identifier == "source_site") {
		effect = std::make_unique<source_site_effect<scope_type>>(effect_operator);
	} else if (effect_identifier == "tooltip") {
		effect = std::make_unique<tooltip_effect<scope_type>>(effect_operator);
	}

	if (effect == nullptr) {
		throw std::runtime_error("Invalid scope effect: \"" + effect_identifier + "\".");
	}

	database::process_gsml_data(effect, scope);

	return effect;
}

template <typename scope_type>
const country *effect<scope_type>::get_scope_country(const scope_type *scope)
{
	if constexpr (std::is_same_v<scope_type, const character>) {
		return scope->get_game_data()->get_employer();
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		return scope;
	} else if constexpr (std::is_same_v<scope_type, population_unit>) {
		const province *province = scope->get_province();
		if (province != nullptr) {
			return province->get_game_data()->get_owner();
		}

		return nullptr;
	} else if constexpr (std::is_same_v<scope_type, const province>) {
		return scope->get_game_data()->get_owner();
	} else if constexpr (std::is_same_v<scope_type, const site>) {
		const province *province = scope->get_game_data()->get_province();
		if (province != nullptr) {
			return province->get_game_data()->get_owner();
		}

		return nullptr;
	}
}

template <typename scope_type>
effect<scope_type>::effect(const gsml_operator effect_operator) : effect_operator(effect_operator)
{
}

template <typename scope_type>
void effect<scope_type>::process_gsml_property(const gsml_property &property)
{
	throw std::runtime_error("Invalid property for \"" + this->get_class_identifier() + "\" effect: \"" + property.get_key() + "\".");
}

template <typename scope_type>
void effect<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid scope for \"" + this->get_class_identifier() + "\" effect: \"" + scope.get_tag() + "\".");
}

template <typename scope_type>
void effect<scope_type>::do_effect(scope_type *scope, context &ctx) const
{
	switch (this->effect_operator) {
		case gsml_operator::assignment:
			this->do_assignment_effect(scope, ctx);
			break;
		case gsml_operator::addition:
			this->do_addition_effect(scope, ctx);
			break;
		case gsml_operator::subtraction:
			this->do_subtraction_effect(scope, ctx);
			break;
		default:
			throw std::runtime_error("Invalid effect operator: \"" + std::to_string(static_cast<int>(this->effect_operator)) + "\".");
	}
}

template <typename scope_type>
std::string effect<scope_type>::get_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
{
	switch (this->effect_operator) {
		case gsml_operator::assignment:
			return this->get_assignment_string(scope, ctx, indent, prefix);
		case gsml_operator::addition:
			return this->get_addition_string(scope, ctx);
		case gsml_operator::subtraction:
			return this->get_subtraction_string(scope, ctx);
		default:
			throw std::runtime_error("Invalid effect operator: \"" + std::to_string(static_cast<int>(this->effect_operator)) + "\".");
	}
}

template class effect<const character>;
template class effect<const country>;
template class effect<population_unit>;
template class effect<const province>;
template class effect<const site>;

}
