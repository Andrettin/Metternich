#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"
#include "technology/technology.h"

namespace metternich {

class technology_condition final : public condition<country>
{
public:
	explicit technology_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->technology = technology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "technology";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_technology(this->technology);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->technology->get_name() + " technology";
	}

private:
	const metternich::technology *technology = nullptr;
};

}
