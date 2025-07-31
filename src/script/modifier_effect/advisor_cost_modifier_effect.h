#pragma once

#include "country/country.h"
#include "country/country_government.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class advisor_cost_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit advisor_cost_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "advisor_cost_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		if (!scope->get_government()->can_have_advisors_or_appointable_offices()) {
			return;
		}

		scope->get_government()->change_advisor_cost_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		return "Advisor Cost";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}

	virtual bool is_hidden(const country *scope) const override
	{
		return !scope->get_government()->can_have_advisors_or_appointable_offices();
	}
};

}
