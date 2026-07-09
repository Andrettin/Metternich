#include "metternich.h"

#include "script/factor.h"

#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_property.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/factor_modifier.h"

namespace metternich {

template <typename scope_type>
factor<scope_type>::factor()
{
}

template <typename scope_type>
factor<scope_type>::factor(const int base_value) : base_value(decimillesimal_int(base_value))
{
}

template <typename scope_type>
factor<scope_type>::~factor()
{
}

template <typename scope_type>
void factor<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator gsml_operator = property.get_operator();
	const std::string &value = property.get_value();

	if (key == "base_value") {
		if (gsml_operator == gsml_operator::assignment) {
			this->base_value = decimillesimal_int(value);
		} else {
			throw std::runtime_error("Invalid operator for property \"" + key + "\".");
		}
	} else {
		throw std::runtime_error("Invalid factor property: \"" + key + "\".");
	}
}

template <typename scope_type>
void factor<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<factor_modifier<scope_type>>();
		modifier->process_gsml_data(scope);
		this->modifiers.push_back(std::move(modifier));
	} else {
		throw std::runtime_error("Invalid factor scope: \"" + scope.get_tag() + "\".");
	}
}

template <typename scope_type>
void factor<scope_type>::process_gsml_data(const gsml_data &data)
{
	data.process(this);
}

template <typename scope_type>
void factor<scope_type>::check() const
{
	for (const std::unique_ptr<factor_modifier<scope_type>> &modifier : this->modifiers) {
		modifier->check_validity();
	}
}

template <typename scope_type>
decimillesimal_int factor<scope_type>::calculate(const scope_type *scope, const decimillesimal_int &base_value) const
{
	decimillesimal_int value = base_value;

	if (scope != nullptr) {
		for (const std::unique_ptr<factor_modifier<scope_type>> &modifier : this->modifiers) {
			if (modifier->check_conditions(scope)) {
				decimillesimal_int modifier_factor = modifier->get_factor();

				if constexpr (std::is_same_v<scope_type, province>) {
					if (modifier->is_province_level_scaled()) {
						modifier_factor *= scope->get_game_data()->get_level();
					}
				}

				if (modifier->is_additive()) {
					value += modifier_factor;
				} else {
					value *= modifier_factor;
				}
			}
		}
	}

	return value;
}

template <typename scope_type>
decimillesimal_int factor<scope_type>::calculate(const scope_type *scope) const
{
	return this->calculate(scope, this->base_value);
}

template class factor<character>;
template class factor<domain>;
template class factor<population_unit>;
template class factor<province>;
template class factor<site>;

}
