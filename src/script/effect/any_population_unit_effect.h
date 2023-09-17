#pragma once

#include "database/database.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/string_util.h"

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

	virtual void do_assignment_effect(upper_scope_type *upper_scope, context &ctx) const override
	{
		for (const auto &population_unit : upper_scope->get_game_data()->get_population_units()) {
			metternich::population_unit *population_unit_ptr = nullptr;
			if constexpr (std::is_same_v<upper_scope_type, const site>) {
				population_unit_ptr = population_unit.get();
			} else {
				population_unit_ptr = population_unit;
			}

			if (!this->conditions.check(population_unit_ptr, ctx)) {
				continue;
			}

			this->do_scope_effect(population_unit_ptr, upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		std::string str = "Any population unit";

		if constexpr (std::is_same_v<upper_scope_type, const province>) {
			if (ctx.is_root_scope(upper_scope)) {
				str += " in " + string::highlight(upper_scope->get_scope_name());
			}
		}

		return str;
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<population_unit> conditions;
};

}
