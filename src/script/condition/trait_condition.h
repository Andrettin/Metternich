#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "script/condition/condition.h"

namespace metternich {

class trait_condition final : public condition<character>
{
public:
	explicit trait_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->trait = trait::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "trait";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_trait(this->trait);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->trait->get_name() + " trait";
	}

private:
	const metternich::trait *trait = nullptr;
};

}
