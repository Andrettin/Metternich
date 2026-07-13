#pragma once

#include "population/population_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class population_type_bonus_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit population_type_bonus_modifier_effect(const metternich::population_type *population_type, const std::string &value)
		: modifier_effect<scope_type>(value), population_type(population_type)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "population_type_bonus";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(scope_type *scope, const centesimal_int &multiplier) const override
	{
		if (!this->population_type->is_enabled()) {
			co_return;
		}

		co_await scope->get_game_data()->change_population_type_modifier_multiplier(this->population_type, this->value * multiplier / 100);
	}

	virtual std::string get_base_string(scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Bonus", string::get_singular_form(this->population_type->get_name()));
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_hidden(scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return !this->population_type->is_enabled();
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
