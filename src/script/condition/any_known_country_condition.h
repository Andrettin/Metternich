#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_known_country_condition final : public scope_condition_base<country, country>
{
public:
	explicit any_known_country_condition(const gsml_operator condition_operator)
		: scope_condition_base<country, country>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_known_country";
		return class_identifier;
	}

	virtual bool check_assignment(const country *upper_scope, const read_only_context &ctx) const override
	{
		for (const country *known_country : upper_scope->get_game_data()->get_known_countries()) {
			if (this->check_scope(known_country, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any known country";
	}
};

}
