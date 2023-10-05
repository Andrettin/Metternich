#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/consulate.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class free_consulate_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit free_consulate_modifier_effect(const std::string &value)
	{
		this->consulate = consulate::get(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "free_consulate";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_free_consulate_count(this->consulate, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("Free {}", this->consulate->get_name());
	}

	virtual std::string get_string(const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return std::format("Free {} with every known country", this->consulate->get_name());
	}

private:
	const metternich::consulate *consulate = nullptr;
};

}
