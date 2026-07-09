#pragma once

#include "map/province.h"
#include "map/province_map_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_nearby_province_condition final : public scope_condition_base<province, province, read_only_context, condition<province>>
{
public:
	explicit any_nearby_province_condition(const gsml_operator condition_operator)
		: scope_condition_base<province, province, read_only_context, condition<province>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_nearby_province";
		return class_identifier;
	}

	virtual bool check_assignment(const province *upper_scope, const read_only_context &ctx) const override
	{
		for (const province *nearby_province : upper_scope->get_map_data()->get_nearby_provinces()) {
			if (this->check_scope(nearby_province, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any nearby province";
	}
};

}
