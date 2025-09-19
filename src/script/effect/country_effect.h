#pragma once

#include "script/effect/scope_effect.h"

namespace metternich {

class domain;

template <typename upper_scope_type>
class country_effect final : public scope_effect<upper_scope_type, const domain>
{
public:
	explicit country_effect(const gsml_operator effect_operator)
		: scope_effect<upper_scope_type, const domain>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "country";
		return class_identifier;
	}
	
	virtual const domain *get_scope(const upper_scope_type *upper_scope) const override
	{
		return effect<upper_scope_type>::get_scope_country(upper_scope);
	}
};

}
