#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

class is_ruler_condition final : public condition<character>
{
public:
	explicit is_ruler_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "is_ruler";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const character_game_data *character_game_data = scope->get_game_data();

		return this->value == character_game_data->is_ruler();
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		if (this->value) {
			return "Ruler";
		} else {
			return "Not ruler";
		}
	}

private:
	bool value = false;
};

}