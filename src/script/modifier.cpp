#include "metternich.h"

#include "script/modifier.h"

#include "database/gsml_property.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
modifier<scope_type>::modifier()
{
}

template <typename scope_type>
modifier<scope_type>::~modifier()
{
}

template <typename scope_type>
void modifier<scope_type>::process_gsml_property(const gsml_property &property)
{
	std::unique_ptr<modifier_effect<scope_type>> effect = modifier_effect<scope_type>::from_gsml_property(property);
	this->add_modifier_effect(std::move(effect));
}

template <typename scope_type>
void modifier<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	std::unique_ptr<modifier_effect<scope_type>> effect = modifier_effect<scope_type>::from_gsml_scope(scope);
	this->add_modifier_effect(std::move(effect));
}

template <typename scope_type>
void modifier<scope_type>::apply(scope_type *scope, const centesimal_int &multiplier) const
{
	for (const std::unique_ptr<modifier_effect<scope_type>> &modifier_effect : this->modifier_effects) {
		modifier_effect->apply(scope, multiplier);
	}
}

template <typename scope_type>
void modifier<scope_type>::apply(scope_type *scope, const int multiplier) const
{
	this->apply(scope, centesimal_int(multiplier));
}

template <typename scope_type>
void modifier<scope_type>::remove(scope_type *scope, const int multiplier) const
{
	this->apply(scope, centesimal_int(-multiplier));
}

template <typename scope_type>
std::string modifier<scope_type>::get_string(const scope_type *scope, const centesimal_int &multiplier, const size_t indent, const bool ignore_decimals) const
{
	std::string str;
	for (const std::unique_ptr<modifier_effect<scope_type>> &modifier_effect : this->modifier_effects) {
		if (modifier_effect->is_hidden()) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		if (indent > 0) {
			str += std::string(indent, '\t');
		}

		str += modifier_effect->get_string(scope, multiplier, ignore_decimals);
	}
	return str;
}

template <typename scope_type>
std::string modifier<scope_type>::get_string(const scope_type *scope, const int multiplier, const size_t indent) const
{
	return this->get_string(scope, centesimal_int(multiplier), indent);
}

template <typename scope_type>
void modifier<scope_type>::add_modifier_effect(std::unique_ptr<modifier_effect<scope_type>> &&modifier_effect)
{
	this->modifier_effects.push_back(std::move(modifier_effect));
}

template class modifier<const character>;
template class modifier<const country>;
template class modifier<military_unit>;
template class modifier<const province>;
template class modifier<const site>;

}
