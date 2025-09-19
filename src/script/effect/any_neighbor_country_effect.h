#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"

namespace metternich {

class domain;

class any_neighbor_country_effect final : public scope_effect_base<const domain, const domain>
{
public:
	explicit any_neighbor_country_effect(const gsml_operator effect_operator)
		: scope_effect_base<const domain, const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_neighbor_country";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			this->conditions.process_gsml_data(scope);
		} else {
			scope_effect_base<const domain, const domain>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const domain *upper_scope, context &ctx) const override
	{
		for (const domain *neighbor_country : upper_scope->get_game_data()->get_neighbor_countries()) {
			if (!this->conditions.check(neighbor_country, ctx)) {
				continue;
			}

			this->do_scope_effect(neighbor_country, upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Any neighbor country";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<domain> conditions;
};

}
