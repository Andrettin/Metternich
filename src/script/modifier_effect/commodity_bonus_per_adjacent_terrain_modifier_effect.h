#pragma once

#include "economy/commodity.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/terrain_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class commodity_bonus_per_adjacent_terrain_modifier_effect final : public modifier_effect<const site>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_adjacent_terrain";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "commodity") {
			this->commodity = commodity::get(value);
		} else if (key == "terrain") {
			this->terrain = terrain_type::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		const int adjacent_terrain_count = scope->get_map_data()->get_adjacent_terrain_count(this->terrain);
		if (adjacent_terrain_count == 0) {
			return;
		}

		scope->get_game_data()->change_base_commodity_output(this->commodity, this->value * adjacent_terrain_count * multiplier);
	}

	virtual std::string get_base_string() const override
	{
		if (this->commodity->is_storable()) {
			return std::format("{} Output per Adjacent {}", this->commodity->get_name(), this->terrain->get_name());
		} else {
			return std::format("{} per Adjacent {}", this->commodity->get_name(), this->terrain->get_name());
		}
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const metternich::commodity *commodity = nullptr;
	const metternich::terrain_type *terrain = nullptr;
};

}
