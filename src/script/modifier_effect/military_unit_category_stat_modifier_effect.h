#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class military_unit_category_stat_modifier_effect final : public modifier_effect<scope_type>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_category_stat_modifier";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "category") {
			this->category = enum_converter<military_unit_category>::to_enum(value);
		} else if (key == "stat") {
			this->stat = enum_converter<military_unit_stat>::to_enum(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(scope_type *scope, const centesimal_int &multiplier) const override
	{
		for (const military_unit_type *military_unit_type : military_unit_type::get_all()) {
			if (military_unit_type->get_category() == this->category) {
				if constexpr (std::is_same_v<scope_type, const character>) {
					scope->get_game_data()->change_commanded_military_unit_type_stat_modifier(military_unit_type, this->stat, this->value * multiplier);
				} else {
					scope->get_game_data()->change_military_unit_type_stat_modifier(military_unit_type, this->stat, this->value * multiplier);
				}
			}
		}
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} {}", get_military_unit_category_name(this->category), get_military_unit_stat_name(this->stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_military_unit_stat(this->stat);
	}

private:
	military_unit_category category{};
	military_unit_stat stat{};
};

}
