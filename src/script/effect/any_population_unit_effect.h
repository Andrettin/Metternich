#pragma once

#include "database/database.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"

namespace metternich {

class population_unit;

template <typename upper_scope_type>
class any_population_unit_effect final : public scope_effect_base<upper_scope_type, population_unit>
{
public:
	explicit any_population_unit_effect(const gsml_operator effect_operator)
		: scope_effect_base<upper_scope_type, population_unit>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_population_unit";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			scope_effect_base<upper_scope_type, population_unit>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const upper_scope_type *upper_scope, context &ctx) const override
	{
		for (population_unit *population_unit : upper_scope->get_game_data()->get_population_units()) {
			if (!this->conditions.check(population_unit, ctx)) {
				continue;
			}

			this->do_scope_effect(population_unit, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Any population unit";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<population_unit> conditions;
};

}
