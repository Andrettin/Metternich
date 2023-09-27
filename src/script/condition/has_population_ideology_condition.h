#pragma once

#include "country/ideology.h"
#include "population/population.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class has_population_ideology_condition final : public condition<scope_type>
{
public:
	explicit has_population_ideology_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->ideology = ideology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_population_ideology";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_population()->get_ideology_counts().contains(this->ideology);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Has {} population ideology", string::highlight(this->ideology->get_name()));
	}

private:
	const metternich::ideology *ideology = nullptr;
};

}
