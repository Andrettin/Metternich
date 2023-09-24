#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class commodity_percent_effect final : public effect<const country>
{
public:
	explicit commodity_percent_effect(const commodity *commodity, const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator), commodity(commodity)
	{
		this->percent = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "commodity_percent";
		return identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->commodity != nullptr);
	}

	int get_quantity(const country *scope) const
	{
		const int stored_commodity = scope->get_game_data()->get_stored_commodity(this->commodity);
		return stored_commodity * this->percent / 100;
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_stored_commodity(this->commodity, this->get_quantity(scope));
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		scope->get_game_data()->change_stored_commodity(this->commodity, this->get_quantity(scope));
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		scope->get_game_data()->change_stored_commodity(this->commodity, this->get_quantity(scope) * -1);
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

		const int quantity = this->get_quantity(scope);
		if (quantity == 0) {
			return std::string();
		}

		return std::format("Gain {} {}", std::to_string(quantity), string::highlight(this->commodity->get_name()));
	}

	virtual std::string get_subtraction_string(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const int quantity = this->get_quantity(scope);
		if (quantity == 0) {
			return std::string();
		}

		return std::format("Lose {} {}", std::to_string(quantity), string::highlight(this->commodity->get_name()));
	}

private:
	const metternich::commodity *commodity = nullptr;
	int percent = 0;
};

}
