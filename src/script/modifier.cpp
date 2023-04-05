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
void modifier<scope_type>::apply(scope_type *scope, const int multiplier) const
{
	for (const std::unique_ptr<modifier_effect<scope_type>> &modifier_effect : this->modifier_effects) {
		modifier_effect->apply(scope, 1 * multiplier);
	}
}

template <typename scope_type>
void modifier<scope_type>::remove(scope_type *scope, const int multiplier) const
{
	for (const std::unique_ptr<modifier_effect<scope_type>> &modifier_effect : this->modifier_effects) {
		modifier_effect->apply(scope, -1 * multiplier);
	}
}

template <typename scope_type>
std::string modifier<scope_type>::get_string(const int multiplier, const size_t indent) const
{
	std::string str;
	for (size_t i = 0; i < this->modifier_effects.size(); ++i) {
		if (i > 0) {
			str += "\n";
		}

		if (indent > 0) {
			str += std::string(indent, '\t');
		}

		str += this->modifier_effects[i]->get_string(multiplier);
	}
	return str;
}

template <typename scope_type>
int modifier<scope_type>::get_score() const
{
	int score = 0;

	for (const std::unique_ptr<modifier_effect<scope_type>> &modifier_effect : this->modifier_effects) {
		score += modifier_effect->get_score();
	}

	return score;
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

}
