#pragma once

#include "domain/country.h"
#include "domain/country_type.h"
#include "script/condition/condition.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

class country_type_condition final : public condition<country>
{
public:
	explicit country_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->country_type = magic_enum::enum_cast<metternich::country_type>(value).value();
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country_type";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_type() == this->country_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} country type", string::highlight(get_country_type_name(this->country_type)));;
	}

private:
	metternich::country_type country_type;
};

}
