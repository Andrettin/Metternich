#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_modifier_type.h"
#include "character/character_stat.h"
#include "script/modifier_effect/modifier_effect.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class character_stat_modifier_effect final : public modifier_effect<const character>
{
public:
	explicit character_stat_modifier_effect(const character_stat *stat)
		: modifier_effect<const character>(), stat(stat)
	{
	}

	explicit character_stat_modifier_effect(const character_stat *stat, const std::string &value)
		: modifier_effect<const character>(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "character_stat";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "modifier_type") {
			this->modifier_type = magic_enum::enum_cast<character_modifier_type>(value).value();
		} else if (key == "modifier") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const character *scope, const centesimal_int &multiplier) const override
	{
		const character_attribute *attribute = dynamic_cast<const character_attribute *>(this->stat);

		if (attribute != nullptr) {
			if (this->modifier_type.has_value()) {
				if (multiplier > 0) {
					co_await scope->get_game_data()->add_attribute_modifier(attribute, this->modifier_type.value(), this->value.to_int());
				} else if (multiplier < 0) {
					co_await scope->get_game_data()->remove_attribute_modifier(attribute, this->modifier_type.value(), this->value.to_int());
				}
			} else {
				co_await scope->get_game_data()->change_attribute_value(attribute, (this->value * multiplier).to_int());
			}
		} else {
			co_await scope->get_game_data()->change_stat_value(stat, (this->value * multiplier).to_int(), true, false);
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		if (this->modifier_type.has_value()) {
			//FIXME: show "Malus" instead of "Bonus" if the modifier type is a penalty one
			return std::format("{} ({} Bonus)", this->stat->get_name(), get_character_modifier_type_name(this->modifier_type.value()));;
		} else {
			return this->stat->get_name();
		}
	}

private:
	const character_stat *stat = nullptr;
	std::optional<character_modifier_type> modifier_type;
};

}
