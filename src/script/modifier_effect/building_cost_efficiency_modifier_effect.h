#pragma once

#include "culture/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "infrastructure/building_class.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class building_cost_efficiency_modifier_effect final : public modifier_effect<const domain>
{
public:
	building_cost_efficiency_modifier_effect()
	{
	}

	explicit building_cost_efficiency_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "building_cost_efficiency_modifier";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "building_class") {
			this->building_class = metternich::building_class::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		if (this->building_class != nullptr) {
			scope->get_game_data()->change_building_class_cost_efficiency_modifier(this->building_class, (this->value * multiplier).to_int());
		} else {
			scope->get_game_data()->change_building_cost_efficiency_modifier((this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		if (this->building_class != nullptr) {
			const building_type *building = scope->get_game_data()->get_culture()->get_building_class_type(this->building_class);
			return std::format("{} Cost Efficiency", building ? building->get_name() : this->building_class->get_name());
		}

		return "Building Cost Efficiency";
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const metternich::building_class *building_class = nullptr;
};

}
