#pragma once

#include "economy/employment_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

class employment_capacity_modifier_effect final : public modifier_effect<const site>
{
public:
	employment_capacity_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "employment_capacity";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "employment_type") {
			this->employment_type = employment_type::get(value);
		} else if (key == "capacity") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		assert_throw(this->employment_type != nullptr);

		scope->get_game_data()->change_employment_capacity(this->employment_type, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		assert_throw(this->employment_type != nullptr);

		return std::format("{} Employment Capacity", this->employment_type->get_name());
	}

private:
	const metternich::employment_type *employment_type = nullptr;
};

}
