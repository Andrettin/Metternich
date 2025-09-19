#pragma once

#include "domain/country_technology.h"
#include "domain/domain.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

class free_technologies_effect final : public effect<const domain>
{
public:
	explicit free_technologies_effect(const std::string &value, const gsml_operator effect_operator) : effect<const domain>(effect_operator)
	{
		this->count = std::stoi(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "free_technologies";
		return class_identifier;
	}

	virtual void do_assignment_effect(const domain *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		scope->get_technology()->gain_free_technologies(this->count);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Gain {} Free {}", this->count, this->count > 1 ? "Technologies" : "Technology");
	}

private:
	int count = 0;
};

}
