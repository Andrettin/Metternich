#pragma once

#include "economy/commodity.h"
#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class capital_commodity_bonus_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit capital_commodity_bonus_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "capital_commodity_bonus";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_game_data()->get_economy()->change_capital_commodity_bonus(this->commodity, this->value * multiplier);
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		if (this->commodity->is_storable()) {
			return std::format("Capital {} Output", this->commodity->get_name());
		} else {
			return std::format("Capital {}", this->commodity->get_name());
		}
	}

	virtual bool is_hidden(const country *scope) const override
	{
		Q_UNUSED(scope);

		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
