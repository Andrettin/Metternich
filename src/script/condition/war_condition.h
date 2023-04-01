#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <typename scope_type>
class war_condition final : public condition<scope_type>
{
public:
	explicit war_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "war";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const country *country = nullptr;
		if constexpr (std::is_same_v<scope_type, character>) {
			country = scope->get_game_data()->get_country();
		} else {
			country = scope;
		}

		return this->value == country->get_game_data()->at_war();
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "At war";
		} else {
			return "Not at war";
		}
	}

private:
	bool value = false;
};

}
