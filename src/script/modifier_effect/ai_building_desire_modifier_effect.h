#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "infrastructure/building_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/number_util.h"

namespace metternich {

class ai_building_desire_modifier_effect final : public modifier_effect<const country>
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
			this->value = std::stoi(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const country *scope, const int multiplier) const override
	{
		scope->get_game_data()->change_ai_building_desire_modifier(this->building, this->value * multiplier);
	}

	virtual std::string get_string(const int multiplier) const override
	{
		return std::format("AI {} Building Desire: {}", this->building->get_name(), number::to_signed_string(this->value * multiplier));
	}

	virtual int get_score() const override
	{
		return 0;
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const building_type *building = nullptr;
	int value = 0;
};

}