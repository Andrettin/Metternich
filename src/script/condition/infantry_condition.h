#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit.h"
#include "unit/military_unit_type.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

class infantry_condition final : public condition<military_unit>
{
public:
	explicit infantry_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<military_unit>(condition_operator)
	{
		this->infantry = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "infantry";
		return class_identifier;
	}

	virtual bool check_assignment(const military_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_type()->is_infantry() == this->infantry;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight("Infantry");
	}

private:
	bool infantry = false;
};

}
