#pragma once

#include "domain/country_military.h"
#include "domain/domain.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class deployment_limit_modifier_effect final : public modifier_effect<const domain>
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

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_military()->change_deployment_limit((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Deployment Limit";
	}
};

}
