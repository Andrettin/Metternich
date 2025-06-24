#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "population/profession.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class profession_capacity_modifier_effect final : public modifier_effect<const site>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "profession_capacity";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "profession") {
			this->profession = profession::get(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const site *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_profession_capacity(this->profession, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const site *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Capacity", profession->get_name());
	}

	virtual bool is_hidden(const site *scope) const override
	{
		Q_UNUSED(scope);

		for (const population_type *population_type : this->profession->get_population_types()) {
			if (population_type->is_enabled()) {
				return false;
			}
		}

		return true;
	}

private:
	const metternich::profession *profession = nullptr;
};

}
