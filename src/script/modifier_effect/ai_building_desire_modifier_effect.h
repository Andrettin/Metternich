#pragma once

#include "domain/country_ai.h"
#include "domain/domain.h"
#include "infrastructure/building_type.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class ai_building_desire_modifier_effect final : public modifier_effect<const domain>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "ai_building_desire";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "building") {
			this->building = building_type::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_ai()->change_building_desire_modifier(this->building, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("AI {} Building Desire", this->building->get_name());
	}

	virtual bool is_hidden(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return true;
	}

private:
	const building_type *building = nullptr;
};

}
