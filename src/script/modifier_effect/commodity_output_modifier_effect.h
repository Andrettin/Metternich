#pragma once

#include "domain/country.h"
#include "domain/country_economy.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class commodity_output_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_output_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect<scope_type>(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_output_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		if constexpr (std::is_same_v<scope_type, const country>) {
			scope->get_economy()->change_commodity_output_modifier(this->commodity, this->value * multiplier);
		} else {
			scope->get_game_data()->change_commodity_output_modifier(this->commodity, this->value * multiplier);
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Output", this->commodity->get_name());
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_hidden(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
