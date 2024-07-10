#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "population/population_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class population_type_militancy_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit population_type_militancy_modifier_effect(const metternich::population_type *population_type, const std::string &value)
		: modifier_effect(value), population_type(population_type)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "population_type_militancy_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_population_type_militancy_modifier(this->population_type, this->value * multiplier);
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} Militancy Modifier", this->population_type->get_name());
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
