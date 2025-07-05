#pragma once

#include "character/character.h"
#include "character/character_role.h"
#include "script/condition/condition.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class character_role_condition final : public condition<character>
{
public:
	explicit character_role_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->role = magic_enum::enum_cast<character_role>(value);
		assert_throw(this->role.has_value());
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_role";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->has_role(this->role.value());
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} role", get_character_role_name(this->role.value()));
	}

private:
	std::optional<character_role> role;
};

}
