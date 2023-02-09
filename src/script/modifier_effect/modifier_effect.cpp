#include "script/modifier_effect/modifier_effect.h"

#include "character/attribute.h"
#include "character/character.h"
#include "database/gsml_property.h"
#include "script/modifier_effect/attribute_modifier_effect.h"
#include "script/modifier_effect/land_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/loyalty_modifier_effect.h"
#include "script/modifier_effect/naval_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/quarterly_piety_modifier_effect.h"
#include "script/modifier_effect/quarterly_prestige_modifier_effect.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, const character>) {
		if (key == "loyalty") {
			return std::make_unique<loyalty_modifier_effect>(value);
		} else if (enum_converter<attribute>::has_value(key)) {
			return std::make_unique<attribute_modifier_effect>(enum_converter<attribute>::to_enum(key), value);
		}
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		if (key == "land_morale_resistance") {
			return std::make_unique<land_morale_resistance_modifier_effect>(value);
		} else if (key == "naval_morale_resistance") {
			return std::make_unique<naval_morale_resistance_modifier_effect>(value);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, const character> || std::is_same_v<scope_type, const country>) {
		if (key == "quarterly_piety") {
			return std::make_unique<quarterly_piety_modifier_effect<scope_type>>(value);
		} else if (key == "quarterly_prestige") {
			return std::make_unique<quarterly_prestige_modifier_effect<scope_type>>(value);
		}
	}

	throw std::runtime_error("Invalid modifier effect: \"" + key + "\".");
}

template class modifier_effect<const character>;
template class modifier_effect<const country>;
template class modifier_effect<const province>;

}
