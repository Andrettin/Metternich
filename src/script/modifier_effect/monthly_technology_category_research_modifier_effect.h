#pragma once

#include "domain/domain.h"
#include "domain/domain_technology.h"
#include "script/modifier_effect/modifier_effect.h"
#include "technology/technology_category.h"

namespace metternich {

class monthly_technology_category_research_modifier_effect final : public modifier_effect<const domain>
{
public:
	monthly_technology_category_research_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "monthly_technology_category_research";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "category") {
			this->category = technology_category::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_technology()->change_technology_category_monthly_research(this->category, (this->value * multiplier).to_int64());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("Monthly {} Research", this->category->get_name());
	}

private:
	const technology_category *category = nullptr;
};

}
