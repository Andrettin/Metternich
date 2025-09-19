#pragma once

#include "economy/commodity.h"
#include "domain/country_economy.h"
#include "domain/domain.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class capital_commodity_output_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit capital_commodity_output_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "capital_commodity_output_modifier";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_economy()->change_capital_commodity_output_modifier(this->commodity, this->value * multiplier);
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Capital {} Output", this->commodity->get_name());
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_hidden(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
