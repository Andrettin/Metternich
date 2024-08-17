#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <typename scope_type>
class provincial_capital_condition final : public condition<scope_type>
{
public:
	using value_type = std::conditional_t<std::is_same_v<scope_type, province>, const site *, bool>;

	explicit provincial_capital_condition(const value_type value, const gsml_operator condition_operator = gsml_operator::assignment)
		: condition<scope_type>(condition_operator), value(value)
	{
	}

	explicit provincial_capital_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		if constexpr (std::is_same_v<scope_type, province>) {
			this->value = site::get(value);
		} else {
			this->value = string::to_bool(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "provincial_capital";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, province>) {
			return scope->get_provincial_capital() == this->value;
		} else {
			return scope->get_game_data()->is_provincial_capital();
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if constexpr (std::is_same_v<scope_type, province>) {
			return std::format("{} is the provincial capital", condition<scope_type>::get_object_string(this->value));
		} else {
			if (this->value) {
				return "Provincial capital";
			} else {
				return "Not provincial capital";
			}
		}
	}

private:
	value_type value{};
};

}
