#include "metternich.h"

#include "game/combat.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/party.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "engine_interface.h"
#include "game/game.h"
#include "script/effect/effect_list.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/dice.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/random.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

combat::combat(party *attacking_party, party *defending_party)
	: attacking_party(attacking_party), defending_party(defending_party)
{
	this->map_rect = QRect(QPoint(0, 0), QSize(combat::map_width, combat::map_height));
	this->tiles.resize(this->get_map_width() * this->get_map_height());
}

combat::~combat()
{
}

void combat::set_generated_party(std::unique_ptr<party> &&generated_party)
{
	this->generated_party = std::move(generated_party);
}

QVariantList combat::get_character_infos_qvariant_list() const
{
	return container::to_qvariant_list(this->character_infos);
}

void combat::remove_character_info(const character *character)
{
	for (size_t i = 0; i < this->character_infos.size(); ++i) {
		if (this->character_infos[i]->get_character() == character) {
			this->character_infos.erase(this->character_infos.begin() + i);
			return;
		}
	}
}

void combat::initialize()
{
	const QPoint attacker_start_pos(1, this->get_map_height() / 2);
	const QPoint defender_start_pos(this->get_map_width() - 2, this->get_map_height() / 2);

	this->deploy_characters(this->attacking_party->get_characters(), attacker_start_pos, false);
	this->deploy_characters(this->defending_party->get_characters(), defender_start_pos, true);
}

void combat::deploy_characters(const std::vector<const character *> &characters, const QPoint &start_pos, const bool defenders)
{
	std::vector<QPoint> tiles_to_check{ start_pos };
	size_t last_check_index = 0;

	for (const character *character : characters) {
		for (size_t i = last_check_index; i < tiles_to_check.size(); ++i) {
			const QPoint &tile_pos = tiles_to_check.at(i);

			if (this->is_tile_attacker_escape(tile_pos) || this->is_tile_defender_escape(tile_pos)) {
				continue;
			}

			combat_tile &tile = this->get_tile(tile_pos);
			if (tile.character != nullptr) {
				point::for_each_adjacent(tile_pos, [&](const QPoint &adjacent_pos) {
					if (!this->get_map_rect().contains(adjacent_pos)) {
						return;
					}

					if (!vector::contains(tiles_to_check, adjacent_pos)) {
						tiles_to_check.push_back(adjacent_pos);
					}
				});

				continue;
			}

			last_check_index = i;
			tile.character = character;
			auto character_info = make_qunique<combat_character_info>(character, tile_pos, defenders);
			this->character_infos.push_back(std::move(character_info));
			break;
		}
	}
}

void combat::start()
{
	while (!this->attacking_party->get_characters().empty() && !this->defending_party->get_characters().empty()) {
		this->do_round();
	}

	this->result.attacker_victory = this->defending_party->get_characters().empty();

	if (this->result.attacker_victory) {
		this->attacking_party->gain_experience(this->attacker_experience_award);
		this->result.experience_award = this->attacker_experience_award;
	} else {
		this->defending_party->gain_experience(this->defender_experience_award);
		this->result.experience_award = this->defender_experience_award;
	}

	this->process_result();
}

void combat::do_round()
{
	int attacker_initiative = 0;
	int defender_initiative = 0;
	while (attacker_initiative == defender_initiative) {
		attacker_initiative = random::get()->roll_dice(combat::initiative_dice);
		defender_initiative = random::get()->roll_dice(combat::initiative_dice);
	}

	if (attacker_initiative < defender_initiative) {
		this->attacker_experience_award += this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
		if (!this->surprise) {
			this->defender_experience_award += this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
		} else {
			this->surprise = false;
		}
	} else {
		if (!this->surprise) {
			this->defender_experience_award += this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
		} else {
			this->surprise = false;
		}
		this->attacker_experience_award += this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
	}

	std::vector<const character *> all_characters = this->attacking_party->get_characters();
	vector::merge(all_characters, this->defending_party->get_characters());
	for (const character *character : all_characters) {
		character->get_game_data()->decrement_status_effect_rounds();
	}
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
			this->remove_character_info(chosen_enemy);
		}
	}

	return experience_award;
}

void combat::process_result()
{
	const bool success = this->attacking_party->get_domain() == this->scope ? this->result.attacker_victory : !this->result.attacker_victory;

	if (this->scope == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->scope->get_government()->get_war_minister_portrait();

		if (success) {
			std::string effects_string = std::format("Experience: {}", number::to_signed_string(this->result.experience_award));
			if (this->victory_effects != nullptr) {
				const std::string victory_effects_string = this->victory_effects->get_effects_string(this->scope, this->ctx);
				effects_string += "\n" + victory_effects_string;
			}

			engine_interface::get()->add_notification("Victory!", war_minister_portrait, std::format("You have won a combat!\n\n{}", effects_string));
		} else {
			std::string effects_string;
			if (this->defeat_effects != nullptr) {
				const std::string defeat_effects_string = this->defeat_effects->get_effects_string(this->scope, this->ctx);
				effects_string += "\n" + defeat_effects_string;
			}

			engine_interface::get()->add_notification("Defeat!", war_minister_portrait, std::format("You have lost a combat!{}", !effects_string.empty() ? ("\n\n" + effects_string) : ""));
		}
	}

	if (success) {
		if (this->victory_effects != nullptr) {
			this->victory_effects->do_effects(this->scope, this->ctx);
		}
	} else {
		if (this->defeat_effects != nullptr) {
			this->defeat_effects->do_effects(this->scope, this->ctx);
		}
	}
}

combat_tile &combat::get_tile(const QPoint &tile_pos)
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
}

}
