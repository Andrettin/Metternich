#include "metternich.h"

#include "game/combat.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/party.h"
#include "util/assert_util.h"
#include "util/dice.h"
#include "util/random.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

combat::combat(party *attacking_party, party *defending_party)
	: attacking_party(attacking_party), defending_party(defending_party)
{
}

combat::result combat::run()
{
	static constexpr dice initiative_dice(1, 10);

	result result;
	int64_t attacker_experience_award = 0;
	int64_t defender_experience_award = 0;

	bool surprise = this->surprise;

	while (!this->attacking_party->get_characters().empty() && !this->defending_party->get_characters().empty()) {
		int attacker_initiative = 0;
		int defender_initiative = 0;
		while (attacker_initiative == defender_initiative) {
			attacker_initiative = random::get()->roll_dice(initiative_dice);
			defender_initiative = random::get()->roll_dice(initiative_dice);
		}

		if (attacker_initiative < defender_initiative) {
			attacker_experience_award += this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
			if (!surprise) {
				defender_experience_award += this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
			} else {
				surprise = false;
			}
		} else {
			if (!surprise) {
				defender_experience_award += this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
			} else {
				surprise = false;
			}
			attacker_experience_award += this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
		}

		std::vector<const character *> all_characters = this->attacking_party->get_characters();
		vector::merge(all_characters, this->defending_party->get_characters());
		for (const character *character : all_characters) {
			character->get_game_data()->decrement_status_effect_rounds();
		}
	}

	result.attacker_victory = this->defending_party->get_characters().empty();

	if (result.attacker_victory) {
		this->attacking_party->gain_experience(attacker_experience_award);
		result.experience_award = attacker_experience_award;
	} else {
		this->defending_party->gain_experience(defender_experience_award);
		result.experience_award = defender_experience_award;
	}

	return result;
}

int64_t combat::do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier)
{
	if (party->get_characters().empty()) {
		return 0;
	}

	assert_throw(!enemy_party->get_characters().empty());

	int64_t experience_award = 0;

	for (const character *character : party->get_characters()) {
		if (enemy_party->get_characters().empty()) {
			break;
		}

		const metternich::character *chosen_enemy = vector::get_random(enemy_party->get_characters());

		static constexpr dice to_hit_dice(1, 20);
		const int to_hit = 20 - character->get_game_data()->get_to_hit_bonus() - to_hit_modifier;
		const int to_hit_result = to_hit - random::get()->roll_dice(to_hit_dice);

		const int armor_class_bonus = chosen_enemy->get_game_data()->get_armor_class_bonus() + chosen_enemy->get_game_data()->get_species_armor_class_bonus(character->get_species());
		const int armor_class = 10 - armor_class_bonus;
		if (to_hit_result > armor_class) {
			continue;
		}

		const int damage = random::get()->roll_dice(character->get_game_data()->get_damage_dice()) + character->get_game_data()->get_damage_bonus();
		chosen_enemy->get_game_data()->change_hit_points(-damage);

		if (chosen_enemy->get_game_data()->is_dead()) {
			experience_award += chosen_enemy->get_game_data()->get_experience_award();
			enemy_party->remove_character(chosen_enemy);
		}
	}

	return experience_award;
}

}
