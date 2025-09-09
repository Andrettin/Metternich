#pragma once

#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class character_attribute_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit character_attribute_modifier_effect(const character_attribute *attribute, const std::string &value)
		: modifier_effect<const character>(value), attribute(attribute)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "character_attribute";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

private:
	const character_attribute *attribute = nullptr;
};

}
