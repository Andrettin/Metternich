#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "economy/employment_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class output_bonus_per_employee_modifier_effect final : public modifier_effect<scope_type>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "output_bonus_per_employee";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "employment_type") {
			this->employment_type = employment_type::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_employee_output_bonus(this->employment_type, (this->value * multiplier));
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} per {}", this->employment_type->get_output_commodity()->get_name(), this->employment_type->get_name());
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
	const metternich::employment_type *employment_type = nullptr;
};

}
