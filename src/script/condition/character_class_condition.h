#pragma once

#include "character/character.h"
#include "character/character_class.h"
#include "script/condition/condition.h"

namespace metternich {

class character_class_condition final : public condition<character>
{
public:
	explicit character_class_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->character_class = character_class::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_class";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_character_class() == this->character_class;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} character class", this->character_class->get_name());
	}

private:
	const metternich::character_class *character_class = nullptr;
};

}
