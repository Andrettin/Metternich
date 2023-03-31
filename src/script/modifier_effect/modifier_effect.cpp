#include "script/modifier_effect/modifier_effect.h"

#include "character/character.h"
#include "database/gsml_property.h"
#include "script/modifier_effect/air_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/commodity_production_modifier_effect.h"
#include "script/modifier_effect/defense_modifier_effect.h"
#include "script/modifier_effect/land_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/melee_modifier_effect.h"
#include "script/modifier_effect/naval_morale_resistance_modifier_effect.h"
#include "script/modifier_effect/production_modifier_effect.h"
#include "script/modifier_effect/quarterly_piety_modifier_effect.h"
#include "script/modifier_effect/quarterly_prestige_modifier_effect.h"
#include "script/modifier_effect/storage_capacity_modifier_effect.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if constexpr (std::is_same_v<scope_type, const country>) {
		if (key == "air_morale_resistance") {
			return std::make_unique<air_morale_resistance_modifier_effect>(value);
		} else if (key == "land_morale_resistance") {
			return std::make_unique<land_morale_resistance_modifier_effect>(value);
		} else if (key == "naval_morale_resistance") {
			return std::make_unique<naval_morale_resistance_modifier_effect>(value);
		} else if (key == "storage_capacity") {
			return std::make_unique<storage_capacity_modifier_effect>(value);
		}
	} else if constexpr (std::is_same_v<scope_type, military_unit>) {
		if (key == "defense") {
			return std::make_unique<defense_modifier_effect>(value);
		} else if (key == "melee") {
			return std::make_unique<melee_modifier_effect>(value);
		}
	}
	
	if constexpr (std::is_same_v<scope_type, const character> || std::is_same_v<scope_type, const country>) {
		if (key == "quarterly_piety") {
			return std::make_unique<quarterly_piety_modifier_effect<scope_type>>(value);
		} else if (key == "quarterly_prestige") {
			return std::make_unique<quarterly_prestige_modifier_effect<scope_type>>(value);
		}
	}

	if constexpr (std::is_same_v<scope_type, const country> || std::is_same_v<scope_type, const province>) {
		static const std::string production_suffix = "_production";

		if (key == "production_modifier") {
			return std::make_unique<production_modifier_effect<scope_type>>(value);
		} else if (key.ends_with(production_suffix) && commodity::try_get(key.substr(0, key.size() - production_suffix.size())) != nullptr) {
			const commodity *commodity = commodity::get(key.substr(0, key.size() - production_suffix.size()));
			return std::make_unique<commodity_production_modifier_effect<scope_type>>(commodity, value);
		}
	}

	throw std::runtime_error("Invalid modifier effect: \"" + key + "\".");
}

template class modifier_effect<const character>;
template class modifier_effect<const country>;
template class modifier_effect<military_unit>;
template class modifier_effect<const province>;

}
