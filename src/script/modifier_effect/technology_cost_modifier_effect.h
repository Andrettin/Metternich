#pragma once

#include "domain/domain.h"
#include "domain/domain_technology.h"
#include "script/modifier_effect/modifier_effect.h"
#include "technology/technology_category.h"
#include "technology/technology_subcategory.h"

namespace metternich {

class technology_cost_modifier_effect final : public modifier_effect<const domain>
{
public:
	technology_cost_modifier_effect() = default;

	explicit technology_cost_modifier_effect(const std::string &value)
		: modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "technology_cost_modifier";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "category") {
			this->category = technology_category::get(value);
		} else if (key == "subcategory") {
			this->subcategory = technology_subcategory::get(value);
		} else if (key == "value") {
			this->value = decimillesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const domain *scope, const decimillesimal_int &multiplier) const override
	{
		if (this->subcategory != nullptr) {
			scope->get_technology()->change_technology_subcategory_cost_modifier(this->subcategory, centesimal_int(this->value * multiplier));
		} else if (this->category != nullptr) {
			scope->get_technology()->change_technology_category_cost_modifier(this->category, centesimal_int(this->value * multiplier));
		} else {
			scope->get_technology()->change_technology_cost_modifier(centesimal_int(this->value * multiplier));
		}
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		if (this->category != nullptr) {
			return std::format("{} Technology Cost", this->category->get_name());
		} else if (this->subcategory != nullptr) {
			return std::format("{} Technology Cost", this->subcategory->get_name());
		} else {
			return "Technology Cost";
		}
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_negative(const decimillesimal_int &multiplier) const override
	{
		return (this->value * multiplier) > 0;
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

private:
	const technology_category *category = nullptr;
	const technology_subcategory *subcategory = nullptr;
};

}
