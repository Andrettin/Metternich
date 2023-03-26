#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/trait.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "unit/military_unit.h"
#include "util/string_util.h"

namespace metternich {

class character;

class gain_item_effect final : public effect<const country>
{
public:
	explicit gain_item_effect(const std::string &value, const gsml_operator effect_operator) : effect<const country>(effect_operator)
	{
		this->item = trait::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "gain_spell_scroll";
		return class_identifier;
	}

	virtual void check() const override
	{
		assert_throw(this->item != nullptr);
		assert_throw(this->item->is_item());
	}

	const character *get_best_character(const country *scope, const read_only_context &ctx) const
	{
		//get a suitable character from the ones participating in the adventure (if any)
		//otherwise, try any of the country's characters
		std::vector<const character *> potential_characters;

		for (const military_unit *military_unit : ctx.attacking_military_units) {
			if (military_unit->get_owner() != scope) {
				continue;
			}

			if (military_unit->get_character() == nullptr) {
				continue;
			}

			if (military_unit->get_character()->get_game_data()->can_have_trait(this->item)) {
				potential_characters.push_back(military_unit->get_character());
			}
		}

		for (const military_unit *military_unit : ctx.defending_military_units) {
			if (military_unit->get_owner() != scope) {
				continue;
			}

			if (military_unit->get_character() == nullptr) {
				continue;
			}

			if (military_unit->get_character()->get_game_data()->can_have_trait(this->item)) {
				potential_characters.push_back(military_unit->get_character());
			}
		}

		if (!potential_characters.empty()) {
			std::sort(potential_characters.begin(), potential_characters.end(), character::skill_compare);
			return potential_characters.front();
		}

		for (const character *character : scope->get_game_data()->get_characters()) {
			if (character->get_game_data()->can_have_trait(this->item)) {
				potential_characters.push_back(character);
			}
		}

		if (!potential_characters.empty()) {
			std::sort(potential_characters.begin(), potential_characters.end(), character::skill_compare);
			return potential_characters.front();
		}

		return nullptr;
	}

	virtual void do_assignment_effect(const country *scope, context &ctx) const override
	{
		const character *character = this->get_best_character(scope, ctx);

		if (character == nullptr) {
			return;
		}

		character->get_game_data()->gain_item(this->item);
	}

	virtual std::string get_assignment_string(const country *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const character *character = this->get_best_character(scope, ctx);

		if (character == nullptr) {
			return std::string();
		}

		return "Gain " + string::get_indefinite_article(this->item->get_name()) + " " + string::highlight(this->item->get_name()) + " (given to " + string::highlight(character->get_full_name()) + ")";
	}

private:
	const trait *item = nullptr;
};

}
