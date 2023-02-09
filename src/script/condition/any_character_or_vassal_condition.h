#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

//scope including both characters in the country and rulers of vassal countries
template <typename upper_scope_type>
class any_character_or_vassal_condition final : public scope_condition_base<upper_scope_type, character>
{
public:
	explicit any_character_or_vassal_condition(const gsml_operator condition_operator)
		: scope_condition_base<upper_scope_type, character>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_character_or_vassal";
		return class_identifier;
	}

	virtual bool check_assignment(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		const country *country = nullptr;

		if constexpr (std::is_same_v<upper_scope_type, character>) {
			if (upper_scope->get_game_data()->is_ruler()) {
				country = upper_scope->get_game_data()->get_employer();
			}
		} else {
			country = upper_scope;
		}

		if (country == nullptr) {
			return false;
		}

		for (const metternich::country *vassal_country : country->get_game_data()->get_vassals()) {
			if (vassal_country->get_game_data()->get_ruler() == nullptr) {
				continue;
			}

			if (this->check_scope(vassal_country->get_game_data()->get_ruler(), ctx)) {
				return true;
			}
		}

		const std::vector<const character *> &characters = country->get_game_data()->get_characters();

		for (const character *character : characters) {
			if (character->get_game_data()->is_ruler()) {
				continue;
			}

			if (this->check_scope(character, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any character or vassal";
	}
};

}
