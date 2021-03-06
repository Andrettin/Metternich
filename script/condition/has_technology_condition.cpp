#include "script/condition/has_technology_condition.h"

#include "holding/holding.h"
#include "holding/holding_slot.h"
#include "map/province.h"
#include "population/population_unit.h"
#include "script/condition/condition_check_base.h"
#include "script/scope_util.h"
#include "technology/technology.h"

namespace metternich {

template <typename T>
has_technology_condition<T>::has_technology_condition(const std::string &technology_identifier, const gsml_operator effect_operator)
	: condition<T>(effect_operator)
{
	this->technology = technology::get(technology_identifier);
}

template <typename T>
bool has_technology_condition<T>::check_assignment(const T *scope) const
{
	const territory *territory = get_scope_territory(scope);
	return territory != nullptr && territory->has_technology(this->technology);
}

template <typename T>
void has_technology_condition<T>::bind_condition_check(condition_check_base &check, const T *scope) const {
	const territory *territory = get_scope_territory(scope);
	if (territory != nullptr) {
		scope->connect(territory, &territory::technologies_changed, scope, [&check](){ check.set_result_recalculation_needed(); }, Qt::ConnectionType::DirectConnection);
	}
}

template <typename T>
std::string has_technology_condition<T>::get_equality_string() const
{
	return "Has the " + this->technology->get_name() + " technology";
}

template <typename T>
std::string has_technology_condition<T>::get_inequality_string() const
{
	return "Does not have the " + this->technology->get_name() + " technology";
}

template class has_technology_condition<holding>;
template class has_technology_condition<holding_slot>;
template class has_technology_condition<population_unit>;
template class has_technology_condition<province>;
template class has_technology_condition<territory>;

}
