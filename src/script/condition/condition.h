#pragma once

#include "script/condition/condition_base.h"
#include "script/context.h"

namespace metternich {

class country;
class province;
struct read_only_context;

template <typename scope_type>
class condition : public condition_base<scope_type, read_only_context>
{
public:
	static std::unique_ptr<const condition_base<scope_type, read_only_context>> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<const condition_base<scope_type, read_only_context>> from_gsml_scope(const gsml_data &scope);

	static const country *get_scope_country(const scope_type *scope);
	static const province *get_scope_province(const scope_type *scope);

	explicit condition(const gsml_operator condition_operator) : condition_base<scope_type, read_only_context>(condition_operator)
	{
	}

	virtual ~condition()
	{
	}
};

}
