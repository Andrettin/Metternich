#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename upper_scope_type>
class any_character_condition final : public scope_condition_base<upper_scope_type, character>
{
public:
	explicit any_character_condition(const gsml_operator condition_operator)
		: scope_condition_base<upper_scope_type, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_character";
		return class_identifier;
	}

	virtual bool check_assignment(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		const country *country = nullptr;

		if constexpr (!std::is_same_v<upper_scope_type, character>) {
			country = upper_scope;
		}

		if (country == nullptr) {
			return false;
		}

		const std::vector<const character *> &characters = country->get_game_data()->get_characters();

		for (const character *character : characters) {
			if (this->check_scope(character, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any character";
	}
};

}
