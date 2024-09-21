#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "population/profession.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class profession_commodity_bonus_modifier_effect final : public modifier_effect<scope_type>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "profession_commodity_bonus";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "profession") {
			this->profession = profession::get(value);
		} else if (key == "commodity") {
			this->commodity = commodity::get(value);
		} else if (key == "value") {
			this->value = decimillesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_profession_commodity_bonus(this->profession, this->commodity, (this->value * multiplier));
	}

	virtual std::string get_base_string() const override
	{
		if (this->commodity == this->profession->get_output_commodity()) {
			return std::format("{} Output", this->profession->get_name());
		} else {
			return std::format("{} {} Output", this->profession->get_name(), this->commodity->get_name());
		}
	}

	virtual std::string get_string(const scope_type *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(scope);

		const decimillesimal_int value = this->value * multiplier;
		const std::string number_str = ignore_decimals && !this->are_decimals_relevant() ? number::to_signed_string(value.to_int()) : value.to_signed_string();
		const QColor &number_color = this->is_negative(multiplier) ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str + (this->is_percent() ? "%" : ""), number_color);

		return std::format("{}: {}", this->get_base_string(), colored_number_str);
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const metternich::profession *profession = nullptr;
	const metternich::commodity *commodity = nullptr;
	decimillesimal_int value;
};

}
