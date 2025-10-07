#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "script/effect/effect.h"

namespace metternich {

class traits_effect final : public effect<const character>
{
public:
	explicit traits_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const character>(effect_operator)
	{
		this->trait = trait::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "traits";
		return identifier;
	}

	virtual void do_addition_effect(const character *scope) const override
	{
		scope->get_game_data()->add_trait(this->trait);
	}

	virtual void do_subtraction_effect(const character *scope) const override
	{
		scope->get_game_data()->remove_trait(this->trait);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain the " + this->trait->get_name() + " trait";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose the " + this->trait->get_name() + " trait";
	}

private:
	const metternich::trait *trait = nullptr;
};

}
