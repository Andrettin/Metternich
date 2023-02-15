#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class character;

class visiting_commander_condition final : public scope_condition<site, character>
{
public:
	explicit visiting_commander_condition(const gsml_operator condition_operator)
		: scope_condition<site, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "visiting_commander";
		return class_identifier;
	}

	virtual const character *get_scope(const site *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return upper_scope->get_game_data()->get_visiting_commander();
	}

	virtual std::string get_scope_name() const override
	{
		return "Visiting Commander";
	}
};

}
