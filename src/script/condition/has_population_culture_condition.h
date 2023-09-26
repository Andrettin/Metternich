#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class has_population_culture_condition final : public condition<scope_type>
{
public:
	explicit has_population_culture_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->culture = culture::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_population_culture";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_population()->get_culture_counts().contains(this->culture);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Has {} population culture", string::highlight(this->culture->get_name()));
	}

private:
	const metternich::culture *culture = nullptr;
};

}
