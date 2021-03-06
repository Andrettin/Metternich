#pragma once

#include "script/effect/effect_list.h"

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/effect/effect.h"

namespace metternich {

template <typename T>
effect_list<T>::effect_list()
{
}

template <typename T>
effect_list<T>::~effect_list()
{
}

template <typename T>
void effect_list<T>::process_gsml_property(const gsml_property &property)
{
	this->effects.push_back(effect<T>::from_gsml_property(property));
}

template <typename T>
void effect_list<T>::process_gsml_scope(const gsml_data &scope)
{
	this->effects.push_back(effect<T>::from_gsml_scope(scope));
}

template <typename T>
void effect_list<T>::do_effects(T *scope, const context &ctx) const
{
	for (const std::unique_ptr<effect<T>> &effect : this->effects) {
		effect->do_effect(scope, ctx);
	}
}

template <typename T>
std::string effect_list<T>::get_effects_string(const T *scope, const read_only_context &ctx, const size_t indent) const
{
	std::string effects_string;
	bool first = true;
	for (const std::unique_ptr<effect<T>> &effect : this->effects) {
		if (effect->is_hidden()) {
			continue;
		}

		const std::string effect_string = effect->get_string(scope, ctx, indent);
		if (effect_string.empty()) {
			continue;
		}

		if (first) {
			first = false;
		} else {
			effects_string += "\n";
		}

		if (indent > 0) {
			effects_string += std::string(indent, '\t');
		}

		effects_string += effect_string;
	}
	return effects_string;
}

template class effect_list<character>;
template class effect_list<holding>;
template class effect_list<province>;

}
