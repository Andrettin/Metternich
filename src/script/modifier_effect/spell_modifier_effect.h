#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "spell/spell.h"
#include "spell/spell_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class spell_modifier_effect final : public modifier_effect<const character>
{
public:
	spell_modifier_effect() = default;

	explicit spell_modifier_effect(const std::string &value)
	{
		this->spell = spell::get(value);
		this->value = decimillesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "spell";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "spell") {
			this->spell = spell::get(value);
		} else if (key == "count") {
			this->value = decimillesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	[[nodiscard]] virtual void apply(const character *scope, const decimillesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_learned_spell_count(this->spell, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return std::string(get_spell_type_name(this->spell->get_type()));
	}

	virtual std::string get_string(const character *scope, const decimillesimal_int &multiplier, const size_t indent, const bool ignore_decimals, const std::string &separator) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(ignore_decimals);
		Q_UNUSED(separator);

		return std::format("{} {}: {}", (this->value * multiplier) > 0 ? "Gain" : "Lose", this->get_base_string(scope), this->spell->get_name());
	}

private:
	const metternich::spell *spell = nullptr;
};

}
