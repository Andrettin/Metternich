#pragma once

#include "country/country.h"
#include "country/country_economy.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class inflation_change_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit inflation_change_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "inflation_change";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_economy()->change_inflation_change(this->value * multiplier);
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		return "Inflation Change";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}
};

}
