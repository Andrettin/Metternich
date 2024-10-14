#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"
#include "unit/military_unit_stat.h"

namespace metternich {

template <typename scope_type>  
class military_unit_stat_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit military_unit_stat_modifier_effect(const military_unit_stat stat, const std::string &value)
		: modifier_effect<scope_type>(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_stat";
		return identifier;
	}

	virtual void apply(scope_type *scope, const centesimal_int &multiplier) const override
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			scope->get_game_data()->change_commanded_military_unit_stat_modifier(this->stat, this->value * multiplier);
		} else {
			scope->change_stat(this->stat, this->value * multiplier);
		}
	}

	virtual std::string get_base_string() const override
	{
		return std::string(get_military_unit_stat_name(this->stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_military_unit_stat(this->stat);
	}

private:
	military_unit_stat stat{};
};

}
