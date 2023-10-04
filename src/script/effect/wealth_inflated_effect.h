#pragma once

#include "script/effect/effect.h"

namespace metternich {

class wealth_inflated_effect final : public effect<const country>
{
public:
	explicit wealth_inflated_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const country>(effect_operator)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "wealth_inflated";
		return identifier;
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_wealth(scope->get_game_data()->get_inflated_value(this->quantity));
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		scope->get_game_data()->change_wealth_inflated(this->quantity);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		scope->get_game_data()->change_wealth_inflated(-this->quantity);
	}

	virtual std::string get_assignment_string(const country *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return std::format("Set {} to {}", string::highlight("Wealth"), scope->get_game_data()->get_inflated_value(this->quantity));
	}

	virtual std::string get_addition_string(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return std::format("Gain {} {}", scope->get_game_data()->get_inflated_value(this->quantity), string::highlight("Wealth"));
	}

	virtual std::string get_subtraction_string(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return std::format("Lose {} {}", scope->get_game_data()->get_inflated_value(this->quantity), string::highlight("Wealth"));
	}

private:
	int quantity = 0;
};

}
