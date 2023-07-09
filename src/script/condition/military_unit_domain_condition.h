#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit.h"
#include "unit/military_unit_domain.h"
#include "util/string_util.h"

namespace metternich {

class military_unit_domain_condition final : public condition<military_unit>
{
public:
	explicit military_unit_domain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<military_unit>(condition_operator)
	{
		this->military_unit_domain = enum_converter<metternich::military_unit_domain>::to_enum(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "military_unit_domain";
		return class_identifier;
	}

	virtual bool check_assignment(const military_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_domain() == this->military_unit_domain;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(get_military_unit_domain_name(this->military_unit_domain));
	}

private:
	metternich::military_unit_domain military_unit_domain = metternich::military_unit_domain::none;
};

}
