#pragma once

#include "domain/country.h"
#include "domain/country_economy.h"
#include "economy/commodity.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class commodity_bonus_per_population_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit commodity_bonus_per_population_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_population";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_economy()->change_commodity_bonus_per_population(this->commodity, this->value * multiplier);
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		if (this->commodity->is_storable()) {
			return std::format("{} Output per Population", this->commodity->get_name());
		} else {
			return std::format("{} per Population", this->commodity->get_name());
		}
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
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
