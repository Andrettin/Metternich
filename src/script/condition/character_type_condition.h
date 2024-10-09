#pragma once

#include "character/character.h"
#include "character/character_type.h"
#include "script/condition/condition.h"

namespace metternich {

class character_type_condition final : public condition<character>
{
public:
	explicit character_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->character_type = character_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_type";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_character_type() == this->character_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} character type", this->character_type->get_name());
	}

private:
	const metternich::character_type *character_type = nullptr;
};

}
