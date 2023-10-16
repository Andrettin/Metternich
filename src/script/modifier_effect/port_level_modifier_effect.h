#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class port_level_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit port_level_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "port_level";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_port_level((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Port Level";
	}
};

}
