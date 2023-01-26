#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/office.h"
#include "character/office_type.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

class has_country_office_condition final : public condition<character>
{
public:
	explicit has_country_office_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<character>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_country_office";
		return class_identifier;
	}

	virtual bool check_assignment(const character *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const character_game_data *character_game_data = scope->get_game_data();

		const bool has_office = character_game_data->is_ruler() || (character_game_data->get_office() != nullptr && character_game_data->get_office()->get_type() == office_type::country);

		return this->value == has_office;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(links_allowed);

		if (this->value) {
			return "Country office";
		} else {
			return "No country office";
		}
	}

private:
	bool value = false;
};

}
