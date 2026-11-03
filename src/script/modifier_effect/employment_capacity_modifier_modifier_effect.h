#pragma once

#include "economy/employment_capacity_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class employment_capacity_modifier_modifier_effect final : public modifier_effect<scope_type>
{
public:
	employment_capacity_modifier_modifier_effect() = default;

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "employment_capacity_modifier";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "capacity_type") {
			this->capacity_type = employment_capacity_type::get(value);
		} else if (key == "modifier") {
			this->value = decimillesimal_int(std::stoll(value));
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(scope_type *scope, const decimillesimal_int &multiplier) const override
	{
		assert_throw(this->capacity_type != nullptr);

		scope->get_game_data()->change_employment_capacity_modifier(this->capacity_type, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		assert_throw(this->capacity_type != nullptr);

		return std::format("{} Employment Capacity", this->capacity_type->get_name());
	}

	virtual bool is_percent() const override
	{
		return true;
	}

private:
	const employment_capacity_type *capacity_type = nullptr;
};

}
