#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "population/population_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/string_util.h"

namespace metternich {

class population_type_bonus_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit population_type_bonus_modifier_effect(const metternich::population_type *population_type, const std::string &value)
		: modifier_effect(value), population_type(population_type)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "population_type_bonus";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_population_type_modifier_multiplier(this->population_type, this->value * multiplier / 100);
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} Bonus", string::get_singular_form(this->population_type->get_name()));
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
