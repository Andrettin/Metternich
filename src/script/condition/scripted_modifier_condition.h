#pragma once

#include "script/condition/condition.h"
#include "script/scripted_character_modifier.h"
#include "script/scripted_province_modifier.h"
#include "script/scripted_site_modifier.h"

namespace metternich {

template <typename scope_type>
class scripted_modifier_condition final : public condition<scope_type>
{
public:
	using scripted_modifier_type = std::conditional_t<std::is_same_v<scope_type, character>, scripted_character_modifier, std::conditional_t<std::is_same_v<scope_type, province>, scripted_province_modifier, std::conditional_t<std::is_same_v<scope_type, site>, scripted_site_modifier, void>>>;

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

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_scripted_modifier(this->modifier);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} modifier", string::highlight(this->modifier->get_name()));
	}

private:
	const scripted_modifier_type *modifier = nullptr;
};

}
