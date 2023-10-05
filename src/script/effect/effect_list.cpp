#include "metternich.h"

#include "script/effect/effect_list.h"

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/effect/effect.h"

namespace metternich {

template <typename scope_type>
effect_list<scope_type>::effect_list()
{
}

template <typename scope_type>
effect_list<scope_type>::~effect_list()
{
}

template <typename scope_type>
void effect_list<scope_type>::process_gsml_property(const gsml_property &property)
{
	this->effects.push_back(effect<scope_type>::from_gsml_property(property));
}

template <typename scope_type>
void effect_list<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->effects.push_back(effect<scope_type>::from_gsml_scope(scope));
}

template <typename scope_type>
void effect_list<scope_type>::check() const
{
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		effect->check();
	}
}

template <typename scope_type>
void effect_list<scope_type>::do_effects(scope_type *scope, context &ctx) const
{
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		effect->do_effect(scope, ctx);
	}
}

template <typename scope_type>
std::string effect_list<scope_type>::get_effects_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix, const bool indent_first_line) const
{
	std::string effects_string;
	bool first = true;
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		if (effect->is_hidden()) {
			continue;
		}

		const std::string effect_string = effect->get_string(scope, ctx, indent, prefix);
		if (effect_string.empty()) {
			continue;
		}

		const bool indented_line = !first || indent_first_line;

		if (first) {
			first = false;
		} else {
			effects_string += "\n";
		}

		if (indent > 0 && indented_line) {
			effects_string += std::string(indent, '\t');
		}

		if (!effect_string.starts_with(prefix)) {
			//add prefix if not already prefixed
			effects_string += prefix;
		}
		effects_string += effect_string;
	}
	return effects_string;
}

template <typename scope_type>
void effect_list<scope_type>::add_effect(std::unique_ptr<effect<scope_type>> &&effect)
{
	this->effects.push_back(std::move(effect));
}

template class effect_list<const character>;
template class effect_list<const country>;
template class effect_list<const site>;

}
