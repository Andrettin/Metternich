#pragma once

#include "character/attribute.h"
#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class character;
enum class attribute;

class attribute_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit attribute_modifier_effect(const metternich::attribute attribute, const std::string &value)
		: attribute(attribute)
	{
		this->quantity = std::stoi(value);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "attribute";
		return identifier;
	}

	virtual void apply(const character *scope, const int change) const override
	{
		scope->get_game_data()->change_attribute_value(this->attribute, this->quantity * change);
	}

	virtual std::string get_string() const override
	{
		return get_attribute_name(this->attribute) + ": " + number::to_signed_string(this->quantity);
	}

private:
	metternich::attribute attribute;
	int quantity = 0;
};

}
