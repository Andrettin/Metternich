#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/policy.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

class policy_effect final : public effect<const country>
{
public:
	explicit policy_effect(const metternich::policy *policy, const std::string &value, const gsml_operator effect_operator)
		: effect<const country>(effect_operator), policy(policy)
	{
		this->value = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "policy";
		return identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->policy != nullptr);
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_policy_value(this->policy, this->value);
	}

	virtual void do_addition_effect(const country *scope) const override
	{
		scope->get_game_data()->change_policy_value(this->policy, this->value);
	}

	virtual void do_subtraction_effect(const country *scope) const override
	{
		scope->get_game_data()->change_policy_value(this->policy, -this->value);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set {} to {}", string::highlight(this->value < 0 ? this->policy->get_left_name() : this->policy->get_right_name()), std::abs(this->value));
	}

	virtual std::string get_addition_string() const override
	{
		return std::format("Gain {} {}", std::abs(this->value), string::highlight(this->value < 0 ? this->policy->get_left_name() : this->policy->get_right_name()));
	}

	virtual std::string get_subtraction_string() const override
	{
		return std::format("Gain {} {}", std::abs(this->value), string::highlight(this->value > 0 ? this->policy->get_left_name() : this->policy->get_right_name()));
	}

private:
	const metternich::policy *policy = nullptr;
	int value = 0;
};

}
