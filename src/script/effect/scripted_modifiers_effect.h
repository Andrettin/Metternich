#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "script/effect/effect.h"
#include "script/scripted_character_modifier.h"

namespace metternich {

template <typename scope_type>
class scripted_modifiers_effect final : public effect<const character>
{
public:
	using scripted_modifier_type = std::conditional_t<std::is_same_v<scope_type, const character>, scripted_character_modifier, void>;

	explicit scripted_modifiers_effect(const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
	}

	explicit scripted_modifiers_effect(const std::string &value, const gsml_operator effect_operator)
		: scripted_modifiers_effect<scope_type>(effect_operator)
	{
		this->modifier = scripted_modifier_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "scripted_modifiers";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "modifier") {
			this->modifier = scripted_modifier_type::get(value);
		} else if (key == "duration") {
			this->duration = std::stoi(value);
		} else if (key == "days") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->days_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		} else if (key == "months") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->months_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		} else if (key == "years") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->years_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		}
	}

	virtual void check() const override
	{
		if (this->get_operator() == gsml_operator::addition && this->duration == 0) {
			throw std::runtime_error("Add scripted modifier effect has no duration.");
		}
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->add_scripted_modifier(this->modifier, this->duration);
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->remove_scripted_modifier(this->modifier);
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain the " + this->modifier->get_name() + " modifier for " + std::to_string(this->duration * defines::get()->get_months_per_turn()) + " months";
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose the " + this->modifier->get_name() + " modifier";
	}

private:
	const scripted_modifier_type *modifier = nullptr;
	int duration = 0;
};

}
