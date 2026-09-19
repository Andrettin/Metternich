#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/save_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class save_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit save_modifier_effect(const save_type *type, const std::string &value)
		: modifier_effect<const character>(value), type(type)
	{
	}

	explicit save_modifier_effect(const std::string &value)
		: modifier_effect<const character>(value), type(nullptr)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "save";
		return identifier;
	}

	virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		if (this->type == nullptr) {
			for (const save_type *save_type : save_type::get_all()) {
				scope->get_game_data()->change_save_bonus(save_type, (this->value * multiplier).to_int());
			}
		} else {
			scope->get_game_data()->change_save_bonus(this->type, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		if (this->type == nullptr) {
			return "Save Bonus";
		}

		return this->type->get_name();
	}

private:
	const save_type *type = nullptr;
};

}
