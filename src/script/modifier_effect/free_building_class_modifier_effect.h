#pragma once

#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "script/modifier.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class free_building_class_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit free_building_class_modifier_effect(const std::string &value)
	{
		this->building_class = building_class::get(value);
		this->value = centesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "free_building_class";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_free_building_class_count(this->building_class, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		const building_type *building = scope->get_culture()->get_building_class_type(this->building_class);

		return std::format("Free Building: {}", building ? building->get_name() : this->building_class->get_name());
	}

	virtual std::string get_string(const domain *scope, const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		Q_UNUSED(multiplier);
		Q_UNUSED(ignore_decimals);

		return this->get_base_string(scope);
	}

private:
	const metternich::building_class *building_class = nullptr;
};

}
