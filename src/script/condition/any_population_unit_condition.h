#pragma once

#include "population/population_unit.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename scope_type>
class any_population_unit_condition final : public scope_condition_base<scope_type, population_unit>
{
public:
	explicit any_population_unit_condition(const gsml_operator condition_operator)
		: scope_condition_base<scope_type, population_unit>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_population_unit";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		if constexpr (std::is_same_v<scope_type, site>) {
			for (const qunique_ptr<population_unit> &population_unit : upper_scope->get_game_data()->get_population_units()) {
				if (this->check_scope(population_unit.get(), ctx)) {
					return true;
				}
			}
		} else {
			for (const population_unit *population_unit : upper_scope->get_game_data()->get_population_units()) {
				if (this->check_scope(population_unit, ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any population unit";
	}
};

}
