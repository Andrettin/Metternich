#pragma once

#include "game/game.h"
#include "game/game_rule.h"
#include "game/game_rules.h"
#include "script/condition/condition.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class game_rule_condition final : public condition<scope_type>
{
public:
	explicit game_rule_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->game_rule = game_rule::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "game_rule";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		return game::get()->get_rules()->get_value(this->game_rule);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} game rule", this->game_rule->get_name());
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const archimedes::game_rule *game_rule = nullptr;
};

}
