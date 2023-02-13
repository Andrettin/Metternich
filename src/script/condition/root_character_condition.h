#pragma once

#include "character/character.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class root_character_condition final : public condition<scope_type>
{
public:
	explicit root_character_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->character = character::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "root_character";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return std::holds_alternative<const metternich::character *>(ctx.root_scope) && std::get<const metternich::character *>(ctx.root_scope) == this->character;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->character->get_name() + " is the root scope";
	}

private:
	const metternich::character *character = nullptr;
};

}
