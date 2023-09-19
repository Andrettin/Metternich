#pragma once

#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <typename scope_type>
class has_route_condition final : public condition<scope_type>
{
public:
	explicit has_route_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_route";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_route();
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "Has route";
		} else {
			return "Has no route";
		}
	}

private:
	bool value = false;
};

}
