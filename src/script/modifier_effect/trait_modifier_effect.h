#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class trait_modifier_effect final : public modifier_effect<const character>
{
public:
	trait_modifier_effect() = default;

	explicit trait_modifier_effect(const std::string &value)
	{
		this->trait = trait::get(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "trait";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "trait") {
			this->trait = trait::get(value);
		} else if (key == "count") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const character *scope, const centesimal_int &multiplier) const override
	{
		co_await scope->get_game_data()->change_trait_count(this->trait, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Trait";
	}

	virtual std::string get_string(const character *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(ignore_decimals);

		return std::format("{} {}: {}", (this->value * multiplier) > 0 ? "Gain" : "Lose", this->get_base_string(scope), this->trait->get_name());
	}

private:
	const metternich::trait *trait = nullptr;
};

}
