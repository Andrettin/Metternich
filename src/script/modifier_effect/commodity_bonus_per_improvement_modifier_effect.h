#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "infrastructure/improvement.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class commodity_bonus_per_improvement_modifier_effect final : public modifier_effect<scope_type>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_bonus_per_improvement";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "commodity") {
			this->commodity = commodity::get(value);
		} else if (key == "improvement") {
			this->improvement = improvement::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_improvement_commodity_bonus(this->improvement, this->commodity, (this->value * multiplier));
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} per {}", this->commodity->get_name(), this->improvement->get_name());
	}

	virtual std::string get_string(const scope_type *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(scope);

		return modifier_effect<scope_type>::get_string(multiplier, ignore_decimals);
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const metternich::commodity *commodity = nullptr;
	const metternich::improvement *improvement = nullptr;
};

}
