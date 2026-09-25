#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "infrastructure/building_type.h"
#include "script/effect/effect.h"

namespace metternich {

class buildings_effect final : public effect<const site>
{
public:
	explicit buildings_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const site>(effect_operator)
	{
		this->building_type = building_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "buildings";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> do_addition_effect_coro(const site *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		co_await scope->get_game_data()->add_building(this->building_type);
	}

	[[nodiscard]] virtual QCoro::Task<void> do_subtraction_effect_coro(const site *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		co_await scope->get_game_data()->remove_building(this->building_type);
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain the {} building", this->building_type->get_name());
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Lose the {} building", this->building_type->get_name());
	}

private:
	const metternich::building_type *building_type = nullptr;
};

}
