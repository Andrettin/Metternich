#pragma once

#include "character/character.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_character_condition final : public scope_condition_base<domain, character, read_only_context, condition<character>>
{
public:
	explicit any_character_condition(const gsml_operator condition_operator)
		: scope_condition_base<domain, character, read_only_context, condition<character>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_character";
		return class_identifier;
	}

	virtual bool check_assignment(const domain *upper_scope, const read_only_context &ctx) const override
	{
		for (const character *character : upper_scope->get_game_data()->get_characters()) {
			if (this->check_scope(character, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any character";
	}
};

}
