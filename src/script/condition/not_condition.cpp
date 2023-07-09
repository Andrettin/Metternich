#include "metternich.h"

#include "script/condition/not_condition.h"

#include "database/gsml_operator.h"

namespace metternich {

template <typename scope_type>
not_condition<scope_type>::not_condition(const gsml_operator condition_operator)
	: condition<scope_type>(condition_operator)
{
}

template <typename scope_type>
not_condition<scope_type>::not_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions)
	: condition<scope_type>(gsml_operator::assignment), conditions(std::move(conditions))
{
}

template <typename scope_type>
not_condition<scope_type>::not_condition(std::unique_ptr<const condition<scope_type>> &&condition)
	: condition<scope_type>(gsml_operator::assignment)
{
	this->conditions.push_back(std::move(condition));
}

template class not_condition<character>;
template class not_condition<country>;
template class not_condition<military_unit>;
template class not_condition<population_unit>;
template class not_condition<province>;
template class not_condition<site>;

}
