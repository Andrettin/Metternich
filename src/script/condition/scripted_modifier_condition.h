#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/condition.h"
#include "script/scripted_character_modifier.h"

namespace metternich {

template <typename scope_type>
class scripted_modifier_condition final : public condition<character>
{
public:
	using scripted_modifier_type = std::conditional_t<std::is_same_v<scope_type, character>, scripted_character_modifier, void>;

	explicit scripted_modifier_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->modifier = scripted_modifier_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_modifier";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_scripted_modifier(this->modifier);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->modifier->get_name() + " modifier";
	}

private:
	const scripted_modifier_type *modifier = nullptr;
};

}
