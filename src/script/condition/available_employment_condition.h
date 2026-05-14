#pragma once

#include "economy/employment_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "population/population_type.h"
#include "script/condition/condition.h"

namespace metternich {

class available_employment_condition final : public condition<site>
{
public:
	explicit available_employment_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->population_type = population_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "available_employment";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		for (const auto &[employment_type, employment_capacity] : scope->get_game_data()->get_employment_capacities()) {
			const metternich::population_type *employed_population_type = nullptr;
			if (!employment_type->can_employ(this->population_type, employed_population_type)) {
				continue;
			}

			if (!employment_type->is_available_for_site(scope)) {
				continue;
			}

			if (scope->get_game_data()->get_available_employment_capacity(employment_type) <= 0) {
				continue;
			}

			if (scope->get_game_data()->get_available_employment_input_capacity(employment_type) <= 0) {
				continue;
			}

			return true;
		}

		return false;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Available employment for {}", string::highlight(this->population_type->get_name()));
	}

private:
	const metternich::population_type *population_type = nullptr;
};

}
