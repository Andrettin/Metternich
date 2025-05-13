#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_subject_country_condition final : public scope_condition_base<country, country, read_only_context, condition<country>>
{
public:
	explicit any_subject_country_condition(const gsml_operator condition_operator)
		: scope_condition_base<country, country, read_only_context, condition<country>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_subject_country";
		return class_identifier;
	}

	virtual bool check_assignment(const country *upper_scope, const read_only_context &ctx) const override
	{
		for (const country *vassal : upper_scope->get_game_data()->get_vassals()) {
			if (this->check_scope(vassal, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any subject country";
	}
};

}
