#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class storage_capacity_modifier_effect final : public modifier_effect<const country>
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

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_storage_capacity((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Storage Capacity";
	}
};

}
