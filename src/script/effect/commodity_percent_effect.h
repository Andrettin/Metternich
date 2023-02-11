#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "economy/commodity.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"

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

	virtual void do_assignment_effect(const country *scope) const override
	{
		const int stored_commodity = scope->get_game_data()->get_stored_commodity(this->commodity);
		scope->get_game_data()->set_stored_commodity(this->commodity, stored_commodity * this->percent / 100);
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		const int stored_commodity = scope->get_game_data()->get_stored_commodity(this->commodity);
		scope->get_game_data()->change_stored_commodity(this->commodity, stored_commodity * this->percent / 100);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		const int stored_commodity = scope->get_game_data()->get_stored_commodity(this->commodity);
		scope->get_game_data()->change_stored_commodity(this->commodity, stored_commodity * this->percent / 100 * -1);
	}

	virtual std::string get_assignment_string() const override
	{
		return "Set " + this->commodity->get_name() + " to " + std::to_string(this->percent) + "%";
	}

	virtual std::string get_addition_string() const override
	{
		return "Gain " + std::to_string(this->percent) + "% " + this->commodity->get_name();
	}

	virtual std::string get_subtraction_string() const override
	{
		return "Lose " + std::to_string(this->percent) + "% " + this->commodity->get_name();
	}

private:
	const metternich::commodity *commodity = nullptr;
	int percent = 0;
};

}
