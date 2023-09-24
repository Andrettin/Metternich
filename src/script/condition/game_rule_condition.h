#pragma once

#include "database/preferences.h"
#include "game/game.h"
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
		this->game_rule = value;

		const QVariant game_rule_variant = preferences::get()->get_game_rules()->property(value.c_str());

		assert_throw(game_rule_variant.isValid());
		assert_throw(game_rule_variant.typeId() == QMetaType::Type::Bool);
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

		const QVariant game_rule_variant = game::get()->get_rules()->property(this->game_rule.c_str());

		assert_throw(game_rule_variant.isValid());
		assert_throw(game_rule_variant.typeId() == QMetaType::Type::Bool);

		return game_rule_variant.toBool();
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->game_rule + " game rule";
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	std::string game_rule;
};

}
