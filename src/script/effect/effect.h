#pragma once

#include "script/effect/effect_base.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
	enum class gsml_operator;
}

namespace metternich {

class country;
class event;
class province;
struct context;
struct read_only_context;

//a scripted effect
template <typename scope_type>
class effect : public effect_base<scope_type, context, read_only_context>
{
public:
	static std::unique_ptr<effect> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<effect> from_gsml_scope(const gsml_data &scope, const effect *previous_effect);

	static const country *get_scope_country(const scope_type *scope);
	static const province *get_scope_province(const scope_type *scope);

	explicit effect(const gsml_operator effect_operator) : effect_base<scope_type, context, read_only_context>(effect_operator)
	{
	}

	virtual ~effect()
	{
	}
};

extern template class effect<const character>;
extern template class effect<const country>;
extern template class effect<population_unit>;
extern template class effect<const province>;
extern template class effect<const site>;

}
