#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/string_conversion_util.h"

namespace metternich {

class range_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit range_modifier_effect(const std::string &value)
		: modifier_effect<const character>()
	{
		this->value = decimillesimal_int(string::to_length(value));
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "range";
		return identifier;
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_range((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Range";
	}

	virtual std::string get_number_string(const decimillesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(ignore_decimals);

		const decimillesimal_int value = this->get_multiplied_value(multiplier);
		return std::format("{}{}", value >= 0 ? "+" : "0", string::from_length(value.to_int(), false));
	}
};

}
