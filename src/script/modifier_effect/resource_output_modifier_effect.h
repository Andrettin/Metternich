#pragma once

#include "economy/commodity.h"
#include "economy/commodity_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class resource_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit resource_output_modifier_effect(const std::string &value) : modifier_effect<scope_type>(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "resource_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		for (const commodity *commodity : commodity::get_all()) {
			if (commodity->get_type() == commodity_type::resource && !commodity->is_abstract()) {
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

		return "Resource Output";
	}

	virtual bool is_percent() const override
	{
		return true;
	}
};

}
