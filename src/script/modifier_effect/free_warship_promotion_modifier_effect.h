#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/promotion.h"

namespace metternich {

class free_warship_promotion_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit free_warship_promotion_modifier_effect(const std::string &value)
	{
		this->promotion = promotion::get(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "free_warship_promotion";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_free_warship_promotion_count(this->promotion, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return std::format("Free Warship Promotion: {}", this->promotion->get_name());
	}

	virtual std::string get_string(const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string();
	}

private:
	const metternich::promotion *promotion = nullptr;
};

}
