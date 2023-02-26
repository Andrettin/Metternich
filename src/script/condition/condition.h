#pragma once

#include "script/context.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
	class named_data_entry;
	enum class gsml_operator;
}

namespace metternich {

class country;
class province;
struct read_only_context;

template <typename scope_type>
class condition
{
public:
	static std::unique_ptr<const condition> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<const condition> from_gsml_scope(const gsml_data &scope);

	static std::string get_conditions_string(const std::vector<std::unique_ptr<const condition<scope_type>>> &conditions, const size_t indent)
	{
		std::string conditions_string;
		bool first = true;
		for (const std::unique_ptr<const condition<scope_type>> &condition : conditions) {
			if (condition->is_hidden()) {
				continue;
			}

			const std::string condition_string = condition->get_string(indent);
			if (condition_string.empty()) {
				continue;
			}

			if (first) {
				first = false;
			} else {
				conditions_string += "\n";
			}

			if (indent > 0) {
				conditions_string += std::string(indent, '\t');
			}

			conditions_string += condition_string;
		}
		return conditions_string;
	}

	//get the string for the object of a condition, e.g. the unit type for a unit type condition
	template <typename T>
	static std::string get_object_string(const T *object, const std::string &name_string = "")
	{
		return condition<scope_type>::get_object_highlighted_name(object, name_string);
	}

	static std::string get_object_highlighted_name(const named_data_entry *object, const std::string &name_string);

	static const country *get_scope_country(const scope_type *scope);
	static const province *get_scope_province(const scope_type *scope);

	explicit condition(const gsml_operator condition_operator);

	virtual ~condition()
	{
	}

	virtual const std::string &get_class_identifier() const = 0;

	virtual void process_gsml_property(const gsml_property &property);
	virtual void process_gsml_scope(const gsml_data &scope);

	virtual void check_validity() const
	{
	}

	bool check(const scope_type *scope, const read_only_context &ctx) const;
	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const = 0;

	virtual bool check_equality(const scope_type *scope) const
	{
		Q_UNUSED(scope);

		throw std::runtime_error("The equality operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual bool check_inequality(const scope_type *scope) const
	{
		return !this->check_equality(scope);
	}

	virtual bool check_less_than(const scope_type *scope) const
	{
		Q_UNUSED(scope);

		throw std::runtime_error("The less than operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual bool check_less_than_or_equality(const scope_type *scope) const
	{
		return this->check_equality(scope) || this->check_less_than(scope);
	}

	virtual bool check_greater_than(const scope_type *scope) const
	{
		Q_UNUSED(scope);

		throw std::runtime_error("The greater than operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual bool check_greater_than_or_equality(const scope_type *scope) const
	{
		return this->check_equality(scope) || this->check_greater_than(scope);
	}

	std::string get_string(const size_t indent) const;
	virtual std::string get_assignment_string(const size_t indent) const = 0;

	virtual std::string get_equality_string() const
	{
		throw std::runtime_error("The equality operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual std::string get_inequality_string() const
	{
		throw std::runtime_error("The inequality operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual std::string get_less_than_string() const
	{
		throw std::runtime_error("The less than operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual std::string get_less_than_or_equality_string() const
	{
		throw std::runtime_error("The less than or equality operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual std::string get_greater_than_string() const
	{
		throw std::runtime_error("The greater than operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual std::string get_greater_than_or_equality_string() const
	{
		throw std::runtime_error("The greater than or equality operator is not supported for \"" + this->get_class_identifier() + "\" conditions.");
	}

	virtual bool is_hidden() const
	{
		return false;
	}

private:
	gsml_operator condition_operator;
};

}
