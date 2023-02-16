#include "metternich.h"

#include "script/factor_modifier.h"

#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_property.h"
#include "script/condition/and_condition.h"
#include "script/context.h"

namespace metternich {

template <typename scope_type>
factor_modifier<scope_type>::factor_modifier()
{
	this->conditions = std::make_unique<and_condition<scope_type>>();
}

template <typename scope_type>
factor_modifier<scope_type>::~factor_modifier()
{
}

template <typename scope_type>
void factor_modifier<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator gsml_operator = property.get_operator();
	const std::string &value = property.get_value();

	if (key == "factor") {
		if (gsml_operator == gsml_operator::assignment) {
			this->factor = centesimal_int(value);
		} else {
			throw std::runtime_error("Invalid operator for property (\"" + property.get_key() + "\").");
		}
	} else {
		std::unique_ptr<const condition<scope_type>> condition = metternich::condition<scope_type>::from_gsml_property(property);
		this->conditions->add_condition(std::move(condition));
	}
}

template <typename scope_type>
void factor_modifier<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	std::unique_ptr<const condition<scope_type>> condition = metternich::condition<scope_type>::from_gsml_scope(scope);
	this->conditions->add_condition(std::move(condition));
}

template <typename scope_type>
void factor_modifier<scope_type>::check_validity() const
{
	if (this->factor == 0) {
		throw std::runtime_error("Factor modifier has a factor of 0.");
	}

	this->conditions->check_validity();
}

template <typename scope_type>
bool factor_modifier<scope_type>::check_conditions(const scope_type *scope) const
{
	return this->conditions->check(scope, read_only_context(scope));
}

template class factor_modifier<character>;
template class factor_modifier<country>;
template class factor_modifier<population_unit>;
template class factor_modifier<province>;
template class factor_modifier<site>;

}
