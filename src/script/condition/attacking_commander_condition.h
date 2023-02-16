#pragma once

#include "script/condition/scope_condition.h"
#include "unit/military_unit.h"

namespace metternich {

class character;

template <typename upper_scope_type>
class attacking_commander_condition final : public scope_condition<upper_scope_type, character>
{
public:
	explicit attacking_commander_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "attacking_commander";
		return class_identifier;
	}

	virtual const character *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		if (ctx.attacking_military_units.empty()) {
			return nullptr;
		}

		return military_unit::get_army_commander(ctx.attacking_military_units);
	}

	virtual std::string get_scope_name() const override
	{
		return "Attacking Commander";
	}
};

}
