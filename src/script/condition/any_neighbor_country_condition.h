#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_neighbor_country_condition final : public scope_condition_base<country, country>
{
public:
	explicit any_neighbor_country_condition(const gsml_operator condition_operator)
		: scope_condition_base<country, country>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_neighbor_country";
		return class_identifier;
	}

	virtual bool check_assignment(const country *upper_scope, const read_only_context &ctx) const override
	{
		for (const country *neighbor_country : upper_scope->get_game_data()->get_neighbor_countries()) {
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
