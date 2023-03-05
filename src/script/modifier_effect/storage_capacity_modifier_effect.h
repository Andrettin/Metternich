#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class storage_capacity_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit storage_capacity_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "storage_capacity";
		return identifier;
	}

	virtual void apply(const country *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_storage_capacity(this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return "Storage Capacity: " + number::to_signed_string(this->quantity * multiplier);
	}

private:
	int quantity = 0;
};

}
