#pragma once

#include <util/fractional_int.h>

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
class and_condition;

//a modifier for a factor, e.g. a random chance or weight
template <typename scope_type>
class factor_modifier
{
public:
	factor_modifier();
	~factor_modifier();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check_validity() const;

	const centesimal_int &get_factor() const
	{
		return this->factor;
	}

	bool is_additive() const
	{
		return this->additive;
	}

	bool check_conditions(const scope_type *scope) const;

private:
	centesimal_int factor; //the factor of the modifier itself
	bool additive = false; //whether the modifier is additive instead of multiplicative
	std::unique_ptr<and_condition<scope_type>> conditions; //conditions for whether the modifier is to be applied
};

extern template class factor_modifier<character>;
extern template class factor_modifier<country>;
extern template class factor_modifier<population_unit>;
extern template class factor_modifier<province>;
extern template class factor_modifier<site>;

}
