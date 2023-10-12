#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "population/profession.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class profession_capacity_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit profession_capacity_modifier_effect(const metternich::profession *profession, const std::string &value)
		: profession(profession)
	{
		this->value = centesimal_int(std::stoi(value));
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "profession_capacity";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_profession_capacity(this->profession, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("Max {}", this->profession->get_name());
	}

private:
	const metternich::profession *profession = nullptr;
};

}
