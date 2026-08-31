#pragma once

#include "infrastructure/construction_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class construction_level_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit construction_level_modifier_effect(const construction_type construction_type, const std::string &value)
		: modifier_effect<const site>(value)
	{
		this->construction_type = construction_type;
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "construction_level";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const site *scope, const decimillesimal_int &multiplier) const override
	{
		co_await scope->get_game_data()->change_construction_level(this->construction_type, centesimal_int(this->value * multiplier));
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Level", get_construction_type_name(this->construction_type));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	metternich::construction_type construction_type{};
};

}
