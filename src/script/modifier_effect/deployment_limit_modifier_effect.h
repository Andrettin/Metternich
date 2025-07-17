#pragma once

#include "country/country.h"
#include "country/country_military.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class deployment_limit_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit deployment_limit_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "deployment_limit";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_military()->change_deployment_limit((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		return "Deployment Limit";
	}
};

}
