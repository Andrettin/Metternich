#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/effect/effect.h"

namespace metternich {

class experience_effect final : public effect<const character>
{
public:
	explicit experience_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->quantity = std::stoll(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "experience";
		return identifier;
	}

	virtual void do_assignment_effect(const character *scope) const override
	{
		scope->get_game_data()->set_experience(this->quantity);
	}

	virtual void do_addition_effect(const character *scope) const override
	{
		scope->get_game_data()->change_experience(this->quantity);
	}

	virtual void do_subtraction_effect(const character *scope) const override
	{
		scope->get_game_data()->change_experience(-this->quantity);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set {} to {}", string::highlight("Experience"), number::to_formatted_string(this->quantity));
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain {} {}", number::to_formatted_string(this->quantity), string::highlight("Experience"));
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Lose {} {}", number::to_formatted_string(this->quantity), string::highlight("Experience"));
	}

private:
	int64_t quantity = 0;
};

}
