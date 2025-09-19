#pragma once

#include "domain/country_economy.h"
#include "domain/domain.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class storage_capacity_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit storage_capacity_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "storage_capacity";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_economy()->change_storage_capacity((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return "Storage Capacity";
	}
};

}
