#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "script/condition/condition.h"

namespace metternich {

class can_have_trait_condition final : public condition<character>
{
public:
	explicit can_have_trait_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->trait = trait::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "can_have_trait";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		if (this->trait->get_conditions() == nullptr) {
			return true;
		}

		return this->trait->get_conditions()->check(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Can have the " + this->trait->get_name() + " trait";
	}

private:
	const metternich::trait *trait = nullptr;
};

}
