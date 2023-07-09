#pragma once

#include "script/condition/condition.h"

namespace metternich {

class character;
class country;
class military_unit;
class population_unit;
class province;
class site;

template <typename scope_type>
class and_condition final : public condition<scope_type>
{
public:
	explicit and_condition();
	explicit and_condition(const gsml_operator condition_operator);
	explicit and_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions);

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "and";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check_validity() const override;

	template <typename checked_scope_type>
	bool check_internal(const checked_scope_type scope) const
	{
		for (const auto &condition : this->conditions) {
			if (!condition->check(scope)) {
				return false;
			}
		}

		return true;
	}

	template <typename checked_scope_type>
	bool check_internal(const checked_scope_type scope, const read_only_context &ctx) const
	{
		for (const auto &condition : this->conditions) {
			if (!condition->check(scope, ctx)) {
				return false;
			}
		}

		return true;
	}

	bool check(const scope_type *scope, const read_only_context &ctx) const
	{
		return condition<scope_type>::check(scope, ctx);
	}

	bool check(const scope_type *scope) const
	{
		return this->check(scope, read_only_context(scope));
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->check_internal(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		if (this->conditions.empty()) {
			return std::string();
		}

		if (this->conditions.size() == 1) {
			return this->conditions.front()->get_string(indent);
		}

		std::string str = "All of:\n";
		str += this->get_conditions_string(indent + 1);
		return str;
	}

	std::string get_conditions_string(const size_t indent) const
	{
		return condition<scope_type>::get_conditions_string(this->conditions, indent);
	}

	void add_condition(std::unique_ptr<const condition<scope_type>> &&condition)
	{
		this->conditions.push_back(std::move(condition));
	}

private:
	std::vector<std::unique_ptr<const condition<scope_type>>> conditions; //the conditions of which all should be true
};

extern template class and_condition<character>;
extern template class and_condition<country>;
extern template class and_condition<military_unit>;
extern template class and_condition<population_unit>;
extern template class and_condition<province>;
extern template class and_condition<site>;

}
