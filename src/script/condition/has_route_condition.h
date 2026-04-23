#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/route.h"
#include "map/route_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <typename scope_type>
class has_route_condition final : public condition<scope_type>
{
public:
	explicit has_route_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_route";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const province *province = condition<scope_type>::get_scope_province(scope);
		if (province == nullptr) {
			return false;
		}

		for (const route *route : province->get_routes()) {
			if (route->get_game_data()->is_active()) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "Has route";
		} else {
			return "Has no route";
		}
	}

private:
	bool value = false;
};

}
