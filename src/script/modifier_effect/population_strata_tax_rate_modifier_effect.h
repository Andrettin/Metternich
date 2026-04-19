#pragma once

#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "population/population_strata.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/string_util.h"

namespace metternich {

class population_strata_tax_rate_modifier_effect final : public modifier_effect<const domain>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "population_strata_tax_rate";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "strata") {
			this->strata = magic_enum::enum_cast<population_strata>(value).value();
		} else if (key == "rate") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_economy()->change_population_strata_tax_rate(this->strata, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Strata Tax Rate", string::capitalized(magic_enum::enum_name(this->strata)));
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	population_strata strata{};
};

}
