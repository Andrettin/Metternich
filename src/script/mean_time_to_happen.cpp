#include "metternich.h"

#include "script/mean_time_to_happen.h"

#include "database/defines.h"
#include "database/gsml_property.h"
#include "script/factor.h"

namespace metternich {

template <typename scope_type>
mean_time_to_happen<scope_type>::mean_time_to_happen()
{
	this->factor = std::make_unique<metternich::factor<scope_type>>();
}

template <typename scope_type>
mean_time_to_happen<scope_type>::~mean_time_to_happen()
{
}

template <typename scope_type>
void mean_time_to_happen<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "days") {
		this->months = centesimal_int(value) / 30;
	} else if (key == "months") {
		this->months = centesimal_int(value);
	} else if (key == "years") {
		this->months = centesimal_int(value) * 12;
	} else {
		this->factor->process_gsml_property(property);
	}
}

template <typename scope_type>
void mean_time_to_happen<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->factor->process_gsml_scope(scope);
}

template <typename scope_type>
void mean_time_to_happen<scope_type>::check() const
{
	this->factor->check();
}

template <typename scope_type>
centesimal_int mean_time_to_happen<scope_type>::calculate(const scope_type *scope, const int current_year) const
{
	return this->factor->calculate(scope, defines::get()->months_to_turns(this->months, current_year));
}

template class mean_time_to_happen<character>;
template class mean_time_to_happen<country>;
template class mean_time_to_happen<province>;

}
