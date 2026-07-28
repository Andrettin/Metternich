#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

class immortal_condition final : public condition<character>
{
public:
	explicit immortal_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "immortal";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->is_immortal() == this->value;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return string::highlight("Immortal");
		} else {
			return "Not immortal";
		}
	}

private:
	bool value = false;
};

}
