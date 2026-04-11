#pragma once

#include "script/effect/effect.h"
#include "technology/technology.h"

namespace metternich {

template <typename scope_type>
class technologies_effect final : public effect<scope_type>
{
public:
	explicit technologies_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->technology = technology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "technologies";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> do_addition_effect_coro(const scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, const domain>) {
			co_await scope->get_technology()->add_technology(this->technology);
		} else {
			co_await scope->get_game_data()->add_technology(this->technology);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_subtraction_effect_coro(const scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, const domain>) {
			co_await scope->get_technology()->remove_technology(this->technology);
		} else {
			co_await scope->get_game_data()->remove_technology(this->technology);
		}
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain the " + string::highlight(this->technology->get_name()) + " technology";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose the " + string::highlight(this->technology->get_name()) + " technology";
	}

private:
	const metternich::technology *technology = nullptr;
};

}
