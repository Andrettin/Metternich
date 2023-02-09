#pragma once

#include "database/database.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"

namespace metternich {

class character;

template <typename upper_scope_type>
class any_character_effect final : public scope_effect_base<upper_scope_type, const character>
{
public:
	explicit any_character_effect(const gsml_operator effect_operator)
		: scope_effect_base<upper_scope_type, const character>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_character";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			scope_effect_base<upper_scope_type, const character>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const upper_scope_type *upper_scope, context &ctx) const override
	{
		const country *country = nullptr;

		if constexpr (std::is_same_v<upper_scope_type, const character>) {
			if (upper_scope->get_game_data()->is_ruler()) {
				country = upper_scope->get_game_data()->get_employer();
			}
		} else {
			country = upper_scope;
		}

		if (country == nullptr) {
			return;
		}

		const std::vector<const character *> &characters = country->get_game_data()->get_characters();

		for (const character *character : characters) {
			if (!this->conditions.check(character, ctx)) {
				continue;
			}

			this->do_scope_effect(character, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Any character";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<character> conditions;
};

}
