#pragma once

#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

class capital_condition final : public condition<province>
{
public:
	explicit capital_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<province>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "capital";
		return class_identifier;
	}

	virtual bool check_assignment(const province *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->is_capital();
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "Capital";
		} else {
			return "Not capital";
		}
	}

private:
	bool value = false;
};

}
