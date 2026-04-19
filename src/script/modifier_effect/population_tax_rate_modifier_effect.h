#pragma once

#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "population/population_strata.h"
#include "script/modifier_effect/modifier_effect.h"

#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

class population_tax_rate_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit population_tax_rate_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "population_tax_rate";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		magic_enum::enum_for_each<population_strata>([this, scope, &multiplier](const population_strata strata) {
			scope->get_economy()->change_population_strata_tax_rate(strata, (this->value * multiplier).to_int());
		});
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Population Tax Rate";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
