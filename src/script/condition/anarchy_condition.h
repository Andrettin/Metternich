#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

class anarchy_condition final : public condition<country>
{
public:
	explicit anarchy_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "anarchy";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->is_under_anarchy() == this->value;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "Under anarchy";
		} else {
			return "Not under anarchy";
		}
	}

private:
	bool value = false;
};

}
