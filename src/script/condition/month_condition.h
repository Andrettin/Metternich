#pragma once

#include "game/game.h"
#include "script/condition/numerical_condition.h"
#include "time/month.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

template <typename scope_type>
class month_condition final : public numerical_condition<scope_type, read_only_context>
{
public:
	explicit month_condition(const std::string &value, const gsml_operator condition_operator)
		: numerical_condition<scope_type, read_only_context>(static_cast<int>(magic_enum::enum_cast<month>(value).value()), condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "month";
		return class_identifier;
	}

	virtual int get_scope_value(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return game::get()->get_date().month();
	}

	virtual std::string get_base_value_string() const override
	{
		const month month = static_cast<archimedes::month>(this->get_base_value());
		return string::capitalized(std::string(magic_enum::enum_name(month)));
	}

	virtual std::string get_value_name() const override
	{
		return "is the current month";
	}

	virtual bool check_equality(const scope_type *scope) const override
	{
		if (game::get()->get_current_months_per_turn() >= 12) {
			return true;
		}

		return numerical_condition<scope_type, read_only_context>::check_equality(scope);
	}

	virtual bool check_less_than(const scope_type *scope) const override
	{
		if (game::get()->get_current_months_per_turn() >= 12) {
			return true;
		}

		return numerical_condition<scope_type, read_only_context>::check_less_than(scope);
	}

	virtual bool check_greater_than(const scope_type *scope) const override
	{
		if (game::get()->get_current_months_per_turn() >= 12) {
			return true;
		}

		return numerical_condition<scope_type, read_only_context>::check_greater_than(scope);
	}
};

}
