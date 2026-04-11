#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

class any_neighbor_province_condition final : public scope_condition_base<province, province, read_only_context, condition<province>>
{
public:
	explicit any_neighbor_province_condition(const gsml_operator condition_operator)
		: scope_condition_base<province, province, read_only_context, condition<province>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_neighbor_province";
		return class_identifier;
	}

	virtual bool check_assignment(const province *upper_scope, const read_only_context &ctx) const override
	{
		for (const province *neighbor_province : upper_scope->get_game_data()->get_neighbor_provinces()) {
			if (this->check_scope(neighbor_province, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any neighbor province";
	}
};

}
