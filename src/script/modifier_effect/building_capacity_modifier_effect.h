#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/country_building_slot.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class building_capacity_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit building_capacity_modifier_effect(const metternich::building_slot_type *building_slot_type, const std::string &value)
		: modifier_effect(value), building_slot_type(building_slot_type)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "building_capacity";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		country_building_slot *building_slot = scope->get_game_data()->get_building_slot(this->building_slot_type);
		building_slot->change_capacity((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} Capacity", this->building_slot_type->get_name());
	}

private:
	const metternich::building_slot_type *building_slot_type = nullptr;
};

}
