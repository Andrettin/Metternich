#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit.h"
#include "unit/military_unit_type.h"
#include "util/string_util.h"

namespace metternich {

class military_unit_type_condition final : public condition<military_unit>
{
public:
	explicit military_unit_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<military_unit>(condition_operator)
	{
		this->military_unit_type = military_unit_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "military_unit_type";
		return class_identifier;
	}

	virtual bool check_assignment(const military_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_type() == this->military_unit_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->military_unit_type->get_name());
	}

private:
	const metternich::military_unit_type *military_unit_type = nullptr;
};

}
