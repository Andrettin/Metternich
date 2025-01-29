#pragma once

#include "script/modifier_effect/modifier_effect_base.h"

namespace metternich {

class character;
class country;
class military_unit;
class province;
class site;

template <typename scope_type>
class modifier_effect : public modifier_effect_base<scope_type>
{
public:
	static std::unique_ptr<modifier_effect> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<modifier_effect> from_gsml_scope(const gsml_data &scope);

	modifier_effect()
	{
	}

	explicit modifier_effect(const std::string &value) : modifier_effect_base<scope_type>(value)
	{
	}

	virtual ~modifier_effect()
	{
	}
};

extern template class modifier_effect<const character>;
extern template class modifier_effect<const country>;
extern template class modifier_effect<military_unit>;
extern template class modifier_effect<const province>;
extern template class modifier_effect<const site>;

}
