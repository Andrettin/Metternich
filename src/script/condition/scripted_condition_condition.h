#pragma once

#include "script/condition/condition.h"
#include "script/condition/scripted_condition.h"
#include "script/scope_traits.h"

namespace metternich {

template <typename scope_type>
class scripted_condition_condition final : public condition<scope_type>
{
public:
	explicit scripted_condition_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->scripted_condition = scope_traits_t<scope_type>::scripted_condition_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_condition";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->scripted_condition->get_conditions()->check(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		return this->scripted_condition->get_conditions()->get_string(indent);
	}

private:
	const scripted_condition_base<scope_type, read_only_context, condition<scope_type>> *scripted_condition = nullptr;
};

}
