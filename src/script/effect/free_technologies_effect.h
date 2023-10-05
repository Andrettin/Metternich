#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

class free_technologies_effect final : public effect<const country>
{
public:
	explicit free_technologies_effect(const std::string &value, const gsml_operator effect_operator) : effect<const country>(effect_operator)
	{
		this->count = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "free_technologies";
		return class_identifier;
	}

	virtual void do_assignment_effect(const country *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		scope->get_game_data()->gain_free_technologies(this->count);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Gain {} Free {}", this->count, this->count > 1 ? "Technologies" : "Technology");
	}

private:
	int count = 0;
};

}
