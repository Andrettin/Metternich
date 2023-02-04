#pragma once

#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class numerical_condition : public condition<scope_type>
{
public:
	explicit numerical_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->value = std::stoi(value);
	}

	virtual int get_scope_value(const scope_type *scope) const = 0;

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_greater_than_or_equality(scope);
	}

	virtual bool check_equality(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) == this->value;
	}

	virtual bool check_less_than(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) < this->value;
	}

	virtual bool check_greater_than(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) > this->value;
	}

	virtual std::string get_value_name() const = 0;

	std::string build_string(const std::string &start_str) const
	{
		std::string str = start_str + " " + std::to_string(this->value);

		const std::string value_name = this->get_value_name();
		if (!value_name.empty()) {
			str += " " + value_name;
		}

		return str;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->get_greater_than_or_equality_string();
	}

	virtual std::string get_equality_string() const override
	{
		return std::to_string(this->value) + " " + this->get_value_name();
	}

	virtual std::string get_inequality_string() const override
	{
		return this->build_string("Not");
	}

	virtual std::string get_less_than_string() const override
	{
		return this->build_string("Less than");
	}

	virtual std::string get_less_than_or_equality_string() const override
	{
		return this->build_string("At most");
	}

	virtual std::string get_greater_than_string() const override
	{
		return this->build_string("More than");
	}

	virtual std::string get_greater_than_or_equality_string() const override
	{
		return this->build_string("At least");
	}

private:
	int value = 0;
};

}
