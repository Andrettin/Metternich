#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_throughput_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_throughput_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: commodity(commodity)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_throughput_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_commodity_throughput_modifier(this->commodity, this->quantity * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return this->commodity->get_name() + " Throughput: " + number::to_signed_string(this->quantity * multiplier) + "%";
	}

	virtual int get_score() const override
	{
		return this->quantity;
	}

private:
	const metternich::commodity *commodity = nullptr;
	int quantity = 0;
};

}