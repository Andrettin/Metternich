#pragma once

#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>  
class throughput_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit throughput_modifier_effect(const std::string &value)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "throughput_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_throughput_modifier((this->quantity * multiplier).to_int());
	}

	virtual std::string get_string(const centesimal_int &multiplier) const override
	{
		return std::format("Throughput: {}%", number::to_signed_string((this->quantity * multiplier).to_int()));
	}

	virtual int get_score() const override
	{
		return this->quantity;
	}

private:
	int quantity = 0;
};

}
