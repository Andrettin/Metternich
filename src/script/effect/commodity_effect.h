#pragma once

#include "country/country.h"
#include "country/country_economy.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/commodity_unit.h"
#include "script/effect/effect.h"
#include "util/dice.h"
#include "util/random.h"
#include "util/string_util.h"

namespace metternich {

class commodity_effect final : public effect<const country>
{
public:
	explicit commodity_effect(const metternich::commodity *commodity, const gsml_operator effect_operator)
		: effect<const country>(effect_operator), commodity(commodity)
	{
	}

	explicit commodity_effect(const metternich::commodity *commodity, const std::string &value, const gsml_operator effect_operator)
		: commodity_effect(commodity, effect_operator)
	{
		auto [value_variant, unit] = commodity->string_to_value_variant_with_unit(value);
		this->quantity_variant = std::move(value_variant);
		this->unit = unit;
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "commodity";
		return identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->commodity != nullptr);
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_economy()->set_stored_commodity(this->commodity, this->calculate_quantity());
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		int change = this->calculate_quantity();

		const int storage = scope->get_economy()->get_stored_commodity(this->commodity);
		if (change < 0 && std::abs(change) > storage) {
			change = -storage;
		}

		scope->get_economy()->change_stored_commodity(this->commodity, change);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		int change = -this->calculate_quantity();

		const int storage = scope->get_economy()->get_stored_commodity(this->commodity);
		if (change < 0 && std::abs(change) > storage) {
			change = -storage;
		}

		scope->get_economy()->change_stored_commodity(this->commodity, change);
	}

	int calculate_quantity() const
	{
		int quantity = 0;

		if (std::holds_alternative<dice>(this->quantity_variant)) {
			const dice dice = std::get<archimedes::dice>(this->quantity_variant);

			const int roll_result = random::get()->roll_dice(dice);
			quantity = roll_result;
		} else {
			quantity = std::get<int>(this->quantity_variant);
		}

		if (this->unit != nullptr) {
			quantity *= this->commodity->get_unit_value(this->unit);
		}

		return quantity;
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set {} to {}", string::highlight(this->commodity->get_name()), this->get_quantity_string());
	}

	virtual std::string get_addition_string() const override
	{
		if (this->commodity == defines::get()->get_wealth_commodity()) {
			return std::format("Gain {}", this->get_quantity_string());
		} else {
			return std::format("Gain {} {}", this->get_quantity_string(), string::highlight(this->commodity->get_name()));
		}
	}

	virtual std::string get_subtraction_string() const override
	{
		if (this->commodity == defines::get()->get_wealth_commodity()) {
			return std::format("Lose {}", this->get_quantity_string());
		} else {
			return std::format("Lose {} {}", this->get_quantity_string(), string::highlight(this->commodity->get_name()));
		}
	}

	std::string get_quantity_string() const
	{
		std::string str;

		if (std::holds_alternative<dice>(this->quantity_variant)) {
			const dice dice = std::get<archimedes::dice>(this->quantity_variant);
			str = dice.to_string();
		} else {
			str = number::to_formatted_string(std::get<int>(this->quantity_variant));
		}

		if (this->unit != nullptr) {
			if (this->commodity == defines::get()->get_wealth_commodity()) {
				str += " " + string::highlight(this->unit->get_suffix());
			} else {
				str += " " + this->unit->get_suffix();
			}
		}

		return str;
	}

	virtual bool is_hidden() const override
	{
		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
	std::variant<int, dice> quantity_variant;
	const commodity_unit *unit = nullptr;
};

}
