#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/vector_random_util.h"

namespace metternich {

class domain;

class random_known_domain_effect final : public scope_effect_base<const domain, const domain>
{
public:
	explicit random_known_domain_effect(const gsml_operator effect_operator)
		: scope_effect_base<const domain, const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_known_domain";
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
		std::vector<const domain *> potential_countries;

		for (const domain *known_domain : upper_scope->get_game_data()->get_known_countries()) {
			if (!this->conditions.check(known_domain, ctx)) {
				continue;
			}

			potential_countries.push_back(known_domain);
		}

		if (!potential_countries.empty()) {
			this->do_scope_effect(vector::get_random(potential_countries), upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Random known domain";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<domain> conditions;
};

}
