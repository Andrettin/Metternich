#include "metternich.h"

#include "script/condition/or_condition.h"

#include "database/gsml_operator.h"

namespace metternich {

template <typename scope_type>
or_condition<scope_type>::or_condition(const gsml_operator condition_operator)
	: condition<scope_type>(condition_operator)
{
}

template <typename scope_type>
or_condition<scope_type>::or_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions)
	: condition<scope_type>(gsml_operator::assignment), conditions(std::move(conditions))
{
}

template class or_condition<character>;
template class or_condition<country>;
template class or_condition<military_unit>;
template class or_condition<population_unit>;
template class or_condition<province>;
template class or_condition<site>;

}
