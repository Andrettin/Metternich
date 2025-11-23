#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_tier.h"
#include "domain/domain_tier_data.h"
#include "script/condition/numerical_condition.h"

namespace metternich {

class tier_condition final : public numerical_condition<domain, read_only_context>
{
public:
	explicit tier_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<domain, read_only_context>(static_cast<int>(magic_enum::enum_cast<domain_tier>(value).value()), condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "tier";
		return class_identifier;
	}

	virtual int get_scope_value(const domain *scope) const override
	{
		return static_cast<int>(scope->get_game_data()->get_tier());
	}

	virtual std::string get_base_value_string() const override
	{
		const domain_tier_data *tier_data = domain_tier_data::get(static_cast<domain_tier>(this->get_base_value()));
		assert_throw(tier_data != nullptr);
		return tier_data->get_name();
	}

	virtual std::string get_value_name() const override
	{
		return "tier";
	}
};

}
