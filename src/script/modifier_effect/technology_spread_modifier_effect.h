#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "technology/technology_category.h"

namespace metternich {

class technology_spread_modifier_effect final : public modifier_effect<const province>
{
public:
	technology_spread_modifier_effect() = default;

	explicit technology_spread_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "technology_spread_modifier";
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

	virtual void apply(const province *scope, const centesimal_int &multiplier) const override
	{
		if (this->category != nullptr) {
			scope->get_game_data()->change_technology_category_spread_modifier(this->category, (this->value * multiplier).to_int());
		} else {
			for (const technology_category *category : technology_category::get_all()) {
				scope->get_game_data()->change_technology_category_spread_modifier(category, (this->value * multiplier).to_int());
			}
		}
	}

	virtual std::string get_base_string(const province *scope) const override
	{
		Q_UNUSED(scope);

		if (this->category != nullptr) {
			return std::format("{} Technology Spread", this->category->get_name());
		} else {
			return "Technology Spread";
		}
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const technology_category *category = nullptr;
};

}
