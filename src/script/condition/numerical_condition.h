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
		return "Not " + std::to_string(this->value) + " " + this->get_value_name();
	}

	virtual std::string get_less_than_string() const override
	{
		return "Less than " + std::to_string(this->value) + " " + this->get_value_name();
	}

	virtual std::string get_less_than_or_equality_string() const override
	{
		return "At most " + std::to_string(this->value) + " " + this->get_value_name();
	}

	virtual std::string get_greater_than_string() const override
	{
		return "More than " + std::to_string(this->value) + " " + this->get_value_name();
	}

	virtual std::string get_greater_than_or_equality_string() const override
	{
		return "At least " + std::to_string(this->value) + " " + this->get_value_name();
	}

private:
	int value = 0;
};

}
