#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class character;

class ruler_condition final : public scope_condition<country, character>
{
public:
	explicit ruler_condition(const gsml_operator condition_operator)
		: scope_condition<country, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "ruler";
		return class_identifier;
	}

	virtual const character *get_scope(const country *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_game_data()->get_ruler();
	}

	virtual std::string get_scope_name() const override
	{
		return "Ruler";
	}
};

}
