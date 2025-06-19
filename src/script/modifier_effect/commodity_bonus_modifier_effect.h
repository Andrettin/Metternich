#pragma once

#include "economy/commodity.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class commodity_bonus_modifier_effect final : public modifier_effect<const site>
{
public:
	explicit commodity_bonus_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus";
		return identifier;
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_game_data()->change_base_commodity_output(this->commodity, this->value * multiplier);
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		if (this->commodity->is_storable()) {
			return std::format("{} Output", this->commodity->get_name());
		} else {
			return this->commodity->get_name();
		}
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_hidden() const override
	{
		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
