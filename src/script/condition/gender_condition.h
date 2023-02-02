#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/condition.h"
#include "util/gender.h"

namespace metternich {

class gender_condition final : public condition<character>
{
public:
	explicit gender_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->gender = enum_converter<archimedes::gender>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "gender";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_gender() == this->gender;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return get_gender_name(this->gender);
	}

private:
	archimedes::gender gender = gender::none;
};

}
