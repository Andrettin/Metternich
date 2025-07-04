#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_bonus_per_settlement_modifier_effect final : public modifier_effect<scope_type>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_settlement";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "commodity") {
			this->commodity = commodity::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_game_data()->change_settlement_commodity_bonus(this->commodity, (this->value * multiplier));
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} per Settlement", this->commodity->get_name());
	}

	virtual bool are_decimals_relevant() const override
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
