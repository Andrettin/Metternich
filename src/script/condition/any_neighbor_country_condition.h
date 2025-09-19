#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_neighbor_country_condition final : public scope_condition_base<domain, domain, read_only_context, condition<domain>>
{
public:
	explicit any_neighbor_country_condition(const gsml_operator condition_operator)
		: scope_condition_base<domain, domain, read_only_context, condition<domain>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_neighbor_country";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *upper_scope, const read_only_context &ctx) const override
	{
		for (const domain *neighbor_country : upper_scope->get_game_data()->get_neighbor_countries()) {
			if (this->check_scope(neighbor_country, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any neighbor country";
	}
};

}
