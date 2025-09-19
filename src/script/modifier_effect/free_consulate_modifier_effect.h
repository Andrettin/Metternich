#pragma once

#include "domain/consulate.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class free_consulate_modifier_effect final : public modifier_effect<const domain>
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

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_free_consulate_count(this->consulate, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Free {} with every known country", this->consulate->get_name());
	}

	virtual std::string get_string(const domain *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string(scope);
	}

private:
	const metternich::consulate *consulate = nullptr;
};

}
