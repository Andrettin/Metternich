#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/fractional_int.h"
#include "util/string_util.h"

namespace metternich {

class population_scaled_commodity_effect final : public effect<const country>
{
public:
	explicit population_scaled_commodity_effect(const metternich::commodity *commodity, const std::string &value, const gsml_operator effect_operator)
		: effect<const country>(effect_operator), commodity(commodity)
	{
		this->base_quantity = centesimal_int(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "population_scaled_commodity";
		return identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->commodity != nullptr);
	}

	int get_quantity(const country *scope) const
	{
		return (this->base_quantity * scope->get_game_data()->get_total_unit_count()).to_int();
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_stored_commodity(this->commodity, this->get_quantity(scope));
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		int change = this->get_quantity(scope);

		const int storage = scope->get_game_data()->get_stored_commodity(this->commodity);
		if (change < 0 && std::abs(change) > storage) {
			change = -storage;
		}

		scope->get_game_data()->change_stored_commodity(this->commodity, change);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		int change = -this->get_quantity(scope);

		const int storage = scope->get_game_data()->get_stored_commodity(this->commodity);
		if (change < 0 && std::abs(change) > storage) {
			change = -storage;
		}

		scope->get_game_data()->change_stored_commodity(this->commodity, change);
	}

	virtual std::string get_assignment_string(const country *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return std::format("Set {} to {}", string::highlight(this->commodity->get_name()), std::to_string(this->get_quantity(scope)));
	}

	virtual std::string get_addition_string(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return std::format("Gain {} {}", std::to_string(this->get_quantity(scope)), string::highlight(this->commodity->get_name()));
	}

	virtual std::string get_subtraction_string(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return std::format("Lose {} {}", std::to_string(this->get_quantity(scope)), string::highlight(this->commodity->get_name()));
	}

private:
	const metternich::commodity *commodity = nullptr;
	centesimal_int base_quantity;
};

}
