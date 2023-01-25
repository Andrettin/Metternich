#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class morale_loss_reduction_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit morale_loss_reduction_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "morale_loss_reduction";
		return identifier;
	}

	virtual void apply(const country *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_morale_loss_reduction(this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return "Morale Loss Reduction: " + number::to_signed_string(this->quantity * multiplier) + "%";
	}

private:
	int quantity = 0;
};

}
