#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class max_current_researches_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit max_current_researches_modifier_effect(const std::string &value)
		: modifier_effect<const domain>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "max_current_researches";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_technology()->change_max_current_researches((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Max Current Researches";
	}
};

}
