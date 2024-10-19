#pragma once

#include "script/condition/and_condition_base.h"
#include "script/condition/condition.h"
#include "script/context.h"

namespace metternich {

class character;
class country;
class military_unit;
class province;
class site;

template <typename scope_type>
class and_condition final : public and_condition_base<scope_type, read_only_context, condition<scope_type>>
{
public:
	and_condition()
	{
	}

	explicit and_condition(const gsml_operator condition_operator)
		: and_condition_base<scope_type, read_only_context, condition<scope_type>>(condition_operator)
	{
	}

	explicit and_condition(std::vector<std::unique_ptr<const condition_base<scope_type, read_only_context>>> &&conditions)
		: and_condition_base<scope_type, read_only_context, condition<scope_type>>(std::move(conditions))
	{
	}
};

extern template class and_condition<character>;
extern template class and_condition<country>;
extern template class and_condition<military_unit>;
extern template class and_condition<population_unit>;
extern template class and_condition<province>;
extern template class and_condition<site>;

}
