#pragma once

#include "domain/country_military.h"
#include "domain/domain.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class leader_cost_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit leader_cost_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "leader_cost_modifier";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_military()->change_leader_cost_modifier((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Leader Cost";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}
};

}
