#pragma once

#include "domain/country.h"
#include "domain/country_military.h"
#include "script/modifier.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/promotion.h"

namespace metternich {

class free_artillery_promotion_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit free_artillery_promotion_modifier_effect(const std::string &value)
	{
		this->promotion = promotion::get(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "free_artillery_promotion";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_military()->change_free_artillery_promotion_count(this->promotion, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const country *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Free Artillery Promotion: {}", this->promotion->get_name());
	}

	virtual std::string get_string(const country *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string(scope);
	}

private:
	const metternich::promotion *promotion = nullptr;
};

}
