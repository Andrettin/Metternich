#pragma once

#include "country/country.h"
#include "script/condition/condition.h"

namespace metternich {

class country_condition final : public condition<country>
{
public:
	explicit country_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<metternich::country>(condition_operator)
	{
		this->country = country::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->country == scope;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Is " + this->country->get_name();
	}

private:
	const metternich::country *country = nullptr;
};

}
