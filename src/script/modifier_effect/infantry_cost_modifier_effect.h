#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class infantry_cost_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit infantry_cost_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "infantry_cost_modifier";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_infantry_cost_modifier((this->quantity * multiplier).to_int());
	}

	virtual std::string get_string(const centesimal_int &multiplier) const override
	{
		return std::format("Infantry Cost: {}%", number::to_signed_string((this->quantity * multiplier).to_int()));
	}

	virtual int get_score() const override
	{
		return -this->quantity;
	}

private:
	int quantity = 0;
};

}
