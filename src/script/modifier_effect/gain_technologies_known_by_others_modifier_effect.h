#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/string_conversion_util.h"

namespace metternich {

class gain_technologies_known_by_others_modifier_effect final : public modifier_effect<const country>
{
public:
	explicit gain_technologies_known_by_others_modifier_effect(const std::string &value)
	{
		this->value = centesimal_int(static_cast<int>(string::to_bool(value)));
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "gain_technologies_known_by_others";
		return identifier;
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_gain_technologies_known_by_others_count((this->value * multiplier).to_int());
	}

	virtual std::string get_base_string() const override
	{
		return "Gain technologies acquired by 2 known countries";
	}

	virtual std::string get_string(const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string();
	}
};

}
