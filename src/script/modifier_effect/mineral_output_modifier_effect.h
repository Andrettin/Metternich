#pragma once

#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class mineral_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit mineral_output_modifier_effect(const std::string &value) : modifier_effect<scope_type>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "mineral_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		for (const commodity *commodity : commodity::get_all()) {
			if (commodity->is_mineral()) {
				if constexpr (std::is_same_v<scope_type, const domain>) {
					scope->get_economy()->change_commodity_output_modifier(commodity, this->value * multiplier);
				} else {
					scope->get_game_data()->change_commodity_output_modifier(commodity, this->value * multiplier);
				}
			}
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return "Mineral Output";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
