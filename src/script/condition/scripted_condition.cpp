#include "metternich.h"

#include "script/condition/scripted_condition.h"

#include "script/condition/and_condition.h"

namespace metternich {

template <typename scope_type>
scripted_condition_base<scope_type>::scripted_condition_base()
{
	this->conditions = std::make_unique<and_condition<scope_type>>();
}

template <typename scope_type>
scripted_condition_base<scope_type>::~scripted_condition_base()
{
}

template <typename scope_type>
void scripted_condition_base<scope_type>::process_gsml_property(const gsml_property &property)
{
	this->conditions->process_gsml_property(property);
}

template <typename scope_type>
void scripted_condition_base<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->conditions->process_gsml_scope(scope);
}

template <typename scope_type>
void scripted_condition_base<scope_type>::check() const
{
	this->get_conditions()->check_validity();
}

template class scripted_condition_base<character>;
template class scripted_condition_base<country>;
template class scripted_condition_base<population_unit>;
template class scripted_condition_base<province>;
template class scripted_condition_base<site>;

}
