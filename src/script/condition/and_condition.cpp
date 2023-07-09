#include "metternich.h"

#include "script/condition/and_condition.h"

#include "database/gsml_operator.h"

namespace metternich {

template <typename scope_type>
and_condition<scope_type>::and_condition()
	: condition<scope_type>(gsml_operator::assignment)
{
}

template <typename scope_type>
and_condition<scope_type>::and_condition(const gsml_operator condition_operator)
	: condition<scope_type>(condition_operator)
{
}

template <typename scope_type>
and_condition<scope_type>::and_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions)
	: condition<scope_type>(gsml_operator::assignment), conditions(std::move(conditions))
{
}

template <typename scope_type>
void and_condition<scope_type>::process_gsml_property(const gsml_property &property)
{
	this->conditions.push_back(condition<scope_type>::from_gsml_property(property));
}

template <typename scope_type>
void and_condition<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->conditions.push_back(condition<scope_type>::from_gsml_scope(scope));
}

template <typename scope_type>
void and_condition<scope_type>::check_validity() const
{
	for (const auto &condition : this->conditions) {
		condition->check_validity();
	}
}

template class and_condition<character>;
template class and_condition<country>;
template class and_condition<military_unit>;
template class and_condition<population_unit>;
template class and_condition<province>;
template class and_condition<site>;

}
