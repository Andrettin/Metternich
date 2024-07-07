#pragma once

#include "util/fractional_int.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class population_unit;
class province;
class site;

template <typename scope_type>
class factor_modifier;

//a scripted factor, i.e. a random chance, weight or score
template <typename scope_type>
class factor final
{
public:
	factor();
	explicit factor(const int base_value);
	~factor();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;

	void set_base_value(centesimal_int &&value)
	{
		this->base_value = std::move(value);
	}

	centesimal_int calculate(const scope_type *scope, const centesimal_int &base_value) const;
	centesimal_int calculate(const scope_type *scope) const;

private:
	centesimal_int base_value; //the base value for the factor
	std::vector<std::unique_ptr<factor_modifier<scope_type>>> modifiers; //modifiers for the factor
};

extern template class factor<character>;
extern template class factor<country>;
extern template class factor<population_unit>;
extern template class factor<province>;
extern template class factor<site>;

}
