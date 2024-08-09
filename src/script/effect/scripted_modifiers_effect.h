#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/effect/effect.h"
#include "script/scripted_character_modifier.h"
#include "script/scripted_province_modifier.h"
#include "script/scripted_site_modifier.h"

namespace metternich {

template <typename scope_type>
class scripted_modifiers_effect final : public effect<scope_type>
{
public:
	using scripted_modifier_type = std::conditional_t<std::is_same_v<scope_type, const character>, scripted_character_modifier, std::conditional_t<std::is_same_v<scope_type, const province>, scripted_province_modifier, std::conditional_t<std::is_same_v<scope_type, const site>, scripted_site_modifier, void>>>;

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
			this->months_duration = centesimal_int(value) / 30;
		} else if (key == "months") {
			this->months_duration = centesimal_int(value);
		} else if (key == "years") {
			this->months_duration = centesimal_int(value) * 12;
		}
	}

	virtual void check() const override
	{
		if (this->get_operator() == gsml_operator::addition && this->duration == 0 && this->months_duration == 0) {
			throw std::runtime_error("Add scripted modifier effect has no duration.");
		}
	}

	int get_duration() const
	{
		if (this->duration > 0) {
			return this->duration;
		}

		if (this->months_duration > 0) {
			return centesimal_int::max(defines::get()->months_to_turns(this->months_duration, game::get()->get_year()), 1).to_int();
		}

		return 0;
	}

	virtual void do_addition_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->add_scripted_modifier(this->modifier, this->get_duration());
	}

	virtual void do_subtraction_effect(const scope_type *scope) const override
	{
		scope->get_game_data()->remove_scripted_modifier(this->modifier);
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain the {} modifier for {} turns", string::highlight(this->modifier->get_name()), std::to_string(this->get_duration()));
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Lose the {} modifier", string::highlight(this->modifier->get_name()));
	}

private:
	const scripted_modifier_type *modifier = nullptr;
	int duration = 0;
	centesimal_int months_duration;
};

}
