#pragma once

#include "script/condition/condition.h"
#include "util/fractional_int.h"

namespace metternich {

template <typename scope_type, typename base_value_type = int>
class numerical_condition : public condition<scope_type>
{
public:
	explicit numerical_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		if constexpr (std::is_same_v<base_value_type, centesimal_int>) {
			this->base_value = base_value_type(value);
		} else {
			this->base_value = std::stoi(value);
		}
	}

	virtual int get_scope_value(const scope_type *scope) const = 0;

	const base_value_type &get_base_value() const
	{
		return this->base_value;
	}

	std::string get_base_value_string() const
	{
		if constexpr (std::is_same_v<base_value_type, centesimal_int>) {
			return this->base_value.to_string();
		} else {
			return std::to_string(this->base_value);
		}
	}

	virtual int get_value(const scope_type *scope) const
	{
		Q_UNUSED(scope);

		if constexpr (std::is_same_v<base_value_type, centesimal_int>) {
			return this->base_value.to_int();
		} else {
			return this->base_value;
		}
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return this->check_greater_than_or_equality(scope);
	}

	virtual bool check_equality(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) == this->get_value(scope);
	}

	virtual bool check_less_than(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) < this->get_value(scope);
	}

	virtual bool check_greater_than(const scope_type *scope) const override
	{
		return this->get_scope_value(scope) > this->get_value(scope);
	}

	virtual std::string get_value_name() const = 0;

	std::string build_string(const std::string &start_str) const
	{
		std::string str = start_str + " " + this->get_base_value_string();

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
		return this->get_base_value_string() + " " + this->get_value_name();
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
	base_value_type base_value{};
};

}
