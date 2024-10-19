#include "metternich.h"

#include "script/condition/and_condition.h"

#include "database/gsml_operator.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
void and_condition<scope_type>::process_gsml_property(const gsml_property &property)
{
	this->add_condition(condition<scope_type>::from_gsml_property(property));
}

template <typename scope_type>
void and_condition<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->add_condition(condition<scope_type>::from_gsml_scope(scope));
}

template class and_condition<character>;
template class and_condition<country>;
template class and_condition<military_unit>;
template class and_condition<population_unit>;
template class and_condition<province>;
template class and_condition<site>;

}
