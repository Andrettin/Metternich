#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class unrest_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit unrest_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "unrest";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_unrest((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Unrest";
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}
};

}
