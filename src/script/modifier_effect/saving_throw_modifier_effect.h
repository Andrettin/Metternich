#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/saving_throw_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class saving_throw_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit saving_throw_modifier_effect(const saving_throw_type *type, const std::string &value)
		: modifier_effect<const character>(value), type(type)
	{
	}

	explicit saving_throw_modifier_effect(const std::string &value)
		: modifier_effect<const character>(value), type(nullptr)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "saving_throw";
		return identifier;
	}

	virtual void apply(const character *scope, const centesimal_int &multiplier) const override
	{
		if (this->type == nullptr) {
			for (const saving_throw_type *saving_throw_type : saving_throw_type::get_all()) {
				scope->get_game_data()->change_saving_throw_bonus(saving_throw_type, (this->value * multiplier).to_int());
			}
		} else {
			scope->get_game_data()->change_saving_throw_bonus(this->type, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		if (this->type == nullptr) {
			return "Saving Throw Bonus";
		}

		return this->type->get_name();
	}

private:
	const saving_throw_type *type = nullptr;
};

}
