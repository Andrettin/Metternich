#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "game/game.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/vector_random_util.h"

namespace metternich {

template <typename scope_type>
class random_global_population_unit_effect final : public scope_effect_base<scope_type, population_unit>
{
public:
	explicit random_global_population_unit_effect(const gsml_operator effect_operator)
		: scope_effect_base<scope_type, population_unit>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_global_population_unit";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			scope_effect_base<scope_type, population_unit>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(scope_type *upper_scope, context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		std::vector<population_unit *> potential_population_units;

		for (const country *country : game::get()->get_countries()) {
			for (population_unit *population_unit : country->get_game_data()->get_population_units()) {
				if (!this->conditions.check(population_unit, ctx)) {
					continue;
				}

				assert_throw(population_unit->get_settlement() != nullptr);

				potential_population_units.push_back(population_unit);
			}
		}

		if (!potential_population_units.empty()) {
			this->do_scope_effect(vector::get_random(potential_population_units), upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Random global population unit";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<population_unit> conditions;
};

}
