#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "game/game.h"
#include "population/population_unit.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename scope_type>
class any_global_population_unit_condition final : public scope_condition_base<scope_type, population_unit>
{
public:
	explicit any_global_population_unit_condition(const gsml_operator condition_operator)
		: scope_condition_base<scope_type, population_unit>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_global_population_unit";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		for (const country *country : game::get()->get_countries()) {
			for (const population_unit *population_unit : country->get_game_data()->get_population_units()) {
				if (this->check_scope(population_unit, ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any global population unit";
	}
};

}
