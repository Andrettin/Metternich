#include "metternich.h"

#include "game/combat.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "character/monster_type.h"
#include "character/party.h"
#include "character/skill.h"
#include "character/status_effect.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "item/object_type.h"
#include "item/trap_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "script/effect/effect_list.h"
#include "sound/sound.h"
#include "species/species.h"
#include "spell/spell.h"
#include "spell/spell_target.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/dice.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/point_container.h"
#include "util/point_util.h"
#include "util/random.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

combat::combat(party *attacking_party, party *defending_party, const QSize &map_size)
	: attacking_party(attacking_party), defending_party(defending_party)
{
	this->set_map_size(map_size);

	for (const character *character : this->attacking_party->get_characters()) {
		auto character_info = make_qunique<combat_character_info>(character, false);
		this->character_infos[character] = std::move(character_info);
	}
	for (const character *character : this->defending_party->get_characters()) {
		auto character_info = make_qunique<combat_character_info>(character, true);
		this->character_infos[character] = std::move(character_info);
	}
}

combat::~combat()
{
}

int combat::get_max_range_of_units() const
{
	int max_range = 0;

	for (const character *character : this->attacking_party->get_characters()) {
		max_range = std::max(max_range, character->get_game_data()->get_range());
	}
	for (const character *character : this->defending_party->get_characters()) {
		max_range = std::max(max_range, character->get_game_data()->get_range());
	}

	return max_range;
}

spell_target combat::get_spell_target(const spell *spell) const
{
	return spell->get_target();
}

int combat::get_spell_range(const spell *spell) const
{
	return spell->get_range();
}

void combat::set_generated_party(std::unique_ptr<party> &&generated_party)
{
	this->generated_party = std::move(generated_party);
}

QVariantList combat::get_unit_infos_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->character_infos);
}

combat_character_info *combat::get_character_info(const character *character) const
{
	const auto find_iterator = this->character_infos.find(character);
	assert_throw(find_iterator != this->character_infos.end());
	return find_iterator->second.get();
}

void combat::remove_character_info(const character *character)
{
	const combat_character_info *character_info = this->get_character_info(character);
	const QPoint tile_pos = character_info->get_tile_pos();
	this->get_tile(tile_pos).character = nullptr;
	this->character_infos.erase(character);

	emit tile_unit_changed(tile_pos);
	emit unit_infos_changed();
}

QVariantList combat::get_objects_qvariant_list() const
{
	return container::to_qvariant_list(this->objects);
}

void combat::add_object(const object_type *object_type, const effect_list<const character> *use_effects, const trap_type *trap, const std::string &description, const combat_placement placement, const QPoint &placement_offset)
{
	auto object = make_qunique<combat_object>(this, object_type, use_effects, trap, description, placement, placement_offset);
	this->objects.push_back(std::move(object));
}

void combat::remove_object(const combat_object *object)
{
	assert_throw(object != nullptr);

	const QPoint tile_pos = object->get_tile_pos();
	this->get_tile(tile_pos).object = nullptr;

	for (size_t i = 0; i < this->objects.size(); ++i) {
		if (this->objects[i].get() == object) {
			this->objects.erase(this->objects.begin() + i);
			break;
		}
	}

	emit tile_object_changed(tile_pos);
	emit objects_changed();
}

void combat::initialize()
{
	const size_t tile_count = static_cast<size_t>(this->get_map_width() * this->get_map_height());
	this->tiles.reserve(tile_count);

	for (size_t i = 0; i < tile_count; ++i) {
		this->tiles.emplace_back(this->get_base_terrain(), this->get_base_terrain());
	}

	this->deploy_objects();
	this->deploy_characters(this->attacking_party->get_characters(), false);
	this->deploy_characters(this->defending_party->get_characters(), true);
}

void combat::deploy_objects()
{
	const QPoint left_start_pos(1, (this->get_map_height() - 1) / 2);
	const QPoint right_start_pos(this->get_map_width() - 2, (this->get_map_height() - 1) / 2);
	const QPoint center_start_pos((this->get_map_width() - 1) / 2, (this->get_map_height() - 1) / 2);

	vector::shuffle(this->objects);

	for (const qunique_ptr<combat_object> &object : this->objects) {
		QPoint start_tile_pos;

		switch (object->get_placement()) {
			case combat_placement::left:
				start_tile_pos = left_start_pos;
				break;
			case combat_placement::right:
				start_tile_pos = right_start_pos;
				break;
			case combat_placement::center:
				start_tile_pos = center_start_pos;
				break;
		}

		start_tile_pos += object->get_placement_offset();

		std::vector<QPoint> tiles_to_check = { start_tile_pos };

		for (size_t i = 0; i < tiles_to_check.size(); ++i) {
			const QPoint &tile_pos = tiles_to_check.at(i);

			if (this->is_tile_attacker_escape(tile_pos) || this->is_tile_defender_escape(tile_pos)) {
				continue;
			}

			combat_tile &tile = this->get_tile(tile_pos);
			if (tile.is_occupied()) {
				point::for_each_cardinally_adjacent(tile_pos, [&](const QPoint &adjacent_pos) {
					if (!this->get_map_rect().contains(adjacent_pos)) {
						return;
					}

					if (!vector::contains(tiles_to_check, adjacent_pos)) {
						tiles_to_check.push_back(adjacent_pos);
					}
				}, false, false);

				continue;
			}

			object->set_tile_pos(tile_pos);
			tile.object = object.get();
			break;
		}
	}
}

void combat::deploy_characters(std::vector<const character *> characters, const bool defenders)
{
	const QPoint left_start_pos(1, (this->get_map_height() - 1) / 2);
	const QPoint right_start_pos(this->get_map_width() - 2, (this->get_map_height() - 1) / 2);
	const QPoint center_start_pos((this->get_map_width() - 1) / 2, (this->get_map_height() - 1) / 2);

	std::sort(characters.begin(), characters.end(), [](const character *lhs, const character *rhs) {
		if (lhs->get_game_data()->get_movement() != rhs->get_game_data()->get_movement()) {
			return lhs->get_game_data()->get_movement() < rhs->get_game_data()->get_movement();
		}

		if (lhs->get_game_data()->get_range() != rhs->get_game_data()->get_range()) {
			return lhs->get_game_data()->get_range() > rhs->get_game_data()->get_range();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const character *character : characters) {
		combat_character_info *character_info = this->get_character_info(character);
		assert_throw(character_info != nullptr);

		QPoint start_tile_pos;

		switch (character_info->get_placement()) {
			case combat_placement::left:
				start_tile_pos = left_start_pos;
				break;
			case combat_placement::right:
				start_tile_pos = right_start_pos;
				break;
			case combat_placement::center:
				start_tile_pos = center_start_pos;
				break;
		}

		start_tile_pos += character_info->get_placement_offset();

		std::vector<QPoint> tiles_to_check = { start_tile_pos };

		for (size_t i = 0; i < tiles_to_check.size(); ++i) {
			const QPoint &tile_pos = tiles_to_check.at(i);

			if (this->is_tile_attacker_escape(tile_pos) || this->is_tile_defender_escape(tile_pos)) {
				continue;
			}

			combat_tile &tile = this->get_tile(tile_pos);
			if (tile.is_occupied()) {
				point::for_each_cardinally_adjacent(tile_pos, [&](const QPoint &adjacent_pos) {
					if (!this->get_map_rect().contains(adjacent_pos)) {
						return;
					}

					if (!vector::contains(tiles_to_check, adjacent_pos)) {
						tiles_to_check.push_back(adjacent_pos);
					}
				}, false, !defenders);

				continue;
			}

			tile.character = character;
			character_info->set_tile_pos(tile_pos);
			break;
		}
	}
}

QCoro::Task<void> combat::start_coro()
{
	domain_event::check_events_for_scope(this->scope, event_trigger::combat_started, this->ctx);

	while (!this->attacking_party->get_characters().empty() && (!this->defending_party->get_characters().empty() || !this->objects.empty())) {
		co_await this->do_round();
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

	//resolve all remaining status effects
	std::vector<const character *> all_characters = this->attacking_party->get_characters();
	vector::merge(all_characters, this->defending_party->get_characters());
	for (const character *character : all_characters) {
		while (character->get_game_data()->has_any_status_effect()) {
			character->get_game_data()->decrement_status_effect_rounds();
		}
	}

	emit finished();

	if (this->scope != game::get()->get_player_country()) {
		this->clear();
	}
}

QCoro::Task<void> combat::do_round()
{
	int attacker_initiative = 0;
	int defender_initiative = 0;
	while (attacker_initiative == defender_initiative) {
		attacker_initiative = random::get()->roll_dice(combat::initiative_dice);
		defender_initiative = random::get()->roll_dice(combat::initiative_dice);
	}

	if (attacker_initiative < defender_initiative) {
		this->attacker_experience_award += co_await this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
		if (!this->surprise) {
			this->defender_experience_award += co_await this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
		} else {
			this->surprise = false;
		}
	} else {
		if (!this->surprise) {
			this->defender_experience_award += co_await this->do_party_round(this->defending_party, this->attacking_party, this->defender_to_hit_modifier);
		} else {
			this->surprise = false;
		}
		this->attacker_experience_award += co_await this->do_party_round(this->attacking_party, this->defending_party, this->attacker_to_hit_modifier);
	}

	std::vector<const character *> all_characters = this->attacking_party->get_characters();
	vector::merge(all_characters, this->defending_party->get_characters());
	for (const character *character : all_characters) {
		character->get_game_data()->decrement_status_effect_rounds();
	}
}

QCoro::Task<int64_t> combat::do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier)
{
	if (party->get_characters().empty()) {
		co_return 0;
	}

	if (enemy_party->get_characters().empty() && (party == this->defending_party || this->objects.empty())) {
		co_return 0;
	}

	int64_t experience_award = 0;

	const std::vector<const character *> party_characters = party->get_characters();
	for (const character *character : party_characters) {
		if (enemy_party->get_characters().empty() && (party == this->defending_party || this->objects.empty())) {
			break;
		}

		bool attacked = false;
		combat_character_info *character_info = this->get_character_info(character);
		character_info->set_remaining_movement(character->get_game_data()->get_combat_movement());

		this->set_current_unit(character_info);

		while (!attacked && character_info->get_remaining_movement() > 0) {
			const QPoint current_tile_pos = character_info->get_tile_pos();

			QPoint target_pos(-1, -1);

			if (party->get_domain() == game::get()->get_player_country() && !this->is_autoplay_enabled()) {
				emit movable_tiles_changed();

				target_pos = co_await this->get_target();

				if (this->is_autoplay_enabled()) {
					continue;
				} else if (target_pos == QPoint(-1, -1)) {
					//end unit's turn
					break;
				}
			} else {
				QPoint chosen_target_tile_pos(-1, -1);

				if (!enemy_party->get_characters().empty()) {
					const metternich::character *chosen_enemy = this->choose_enemy(character, enemy_party->get_characters());
					assert_throw(chosen_enemy != nullptr);
					const combat_character_info *chosen_enemy_info = this->get_character_info(chosen_enemy);
					chosen_target_tile_pos = chosen_enemy_info->get_tile_pos();
				} else {
					assert_throw(!this->objects.empty());
					const combat_object *chosen_object = this->choose_target_object(character);
					assert_throw(chosen_object != nullptr);
					chosen_target_tile_pos = chosen_object->get_tile_pos();
				}

				assert_throw(chosen_target_tile_pos != QPoint(-1, -1));

				const int distance_to_target = point::distance_to(current_tile_pos, chosen_target_tile_pos);
				if (distance_to_target <= character_info->get_range()) {
					target_pos = chosen_target_tile_pos;
				} else {
					const int current_square_distance = point::square_distance_to(current_tile_pos, chosen_target_tile_pos);
					int best_square_distance = std::numeric_limits<int>::max();
					std::vector<QPoint> potential_tiles;

					point::for_each_adjacent(current_tile_pos, [&](const QPoint &adjacent_pos) {
						if (!this->get_map_rect().contains(adjacent_pos)) {
							return;
						}

						if (!this->can_current_unit_move_to(adjacent_pos)) {
							return;
						}

						if (this->can_current_unit_retreat_at(adjacent_pos)) {
							return;
						}

						const int square_distance = point::square_distance_to(adjacent_pos, chosen_target_tile_pos);

						if (square_distance >= current_square_distance) {
							return;
						}

						if (square_distance < best_square_distance) {
							best_square_distance = square_distance;
							potential_tiles.clear();
						}

						if (square_distance <= best_square_distance) {
							potential_tiles.push_back(adjacent_pos);
						}
					});

					if (!potential_tiles.empty()) {
						target_pos = vector::get_random(potential_tiles);
					}
				}

				if (target_pos == QPoint(-1, -1)) {
					break;
				}
			}

			const combat_tile &tile = this->get_tile(target_pos);
			const int distance = point::distance_to(current_tile_pos, target_pos);

			if (this->get_current_spell() != nullptr) {
				if (tile.character != nullptr) {
					if (this->get_current_spell()->get_target() == spell_target::enemy && vector::contains(enemy_party->get_characters(), tile.character)) {
						if (distance <= this->get_current_spell()->get_range()) {
							co_await this->do_character_spellcast(character, this->get_current_spell(), tile.character, enemy_party, to_hit_modifier);
							attacked = true;
						}
					} else if (this->get_current_spell()->get_target() == spell_target::ally && vector::contains(party->get_characters(), tile.character)) {
						if (distance <= this->get_current_spell()->get_range()) {
							co_await this->do_character_spellcast(character, this->get_current_spell(), tile.character, party, to_hit_modifier);
							attacked = true;
						}
					}
				}

				this->set_current_spell(nullptr);
			} else {
				if (tile.character != nullptr) {
					if (distance <= character_info->get_range() && vector::contains(enemy_party->get_characters(), tile.character)) {
						experience_award += this->do_character_attack(character, tile.character, enemy_party, to_hit_modifier);
						attacked = true;
					}
				} else if (tile.object != nullptr) {
					if (distance <= 1) {
						//can only use objects (e.g. chests) if the enemy has been wiped out
						if (this->can_current_character_use_object(tile.object)) {
							if (tile.object->get_trap() != nullptr) {
								bool disarmed = false;
								if (tile.object->get_trap_found() && skill::get_disarm_traps_skill() != nullptr) {
									disarmed = character->get_game_data()->do_skill_check(skill::get_disarm_traps_skill(), tile.object->get_trap()->get_disarm_modifier(), this->get_location());
								}

								if (disarmed) {
									if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
										const portrait *war_minister_portrait = character->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();

										engine_interface::get()->add_combat_notification(std::format("{} Disarmed", tile.object->get_trap()->get_name()), war_minister_portrait, std::format("You have disarmed the {} trap!", tile.object->get_trap()->get_name()));
									}
								} else if (tile.object->get_trap()->get_trigger_effects() != nullptr) {
									context ctx = this->ctx;
									ctx.root_scope = character;

									if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
										const portrait *war_minister_portrait = character->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();
										const std::string effects_string = tile.object->get_trap()->get_trigger_effects()->get_effects_string(character, ctx);

										engine_interface::get()->add_combat_notification(std::format("{} Triggered", tile.object->get_trap()->get_name()), war_minister_portrait, effects_string);
									}

									tile.object->get_trap()->get_trigger_effects()->do_effects(character, ctx);
								}

								tile.object->remove_trap();
							}

							if (character->get_game_data()->is_dead()) {
								this->on_character_died(character, party);
								break;
							}

							if (tile.object->get_use_effects() != nullptr) {
								context ctx = this->ctx;
								ctx.root_scope = character;

								if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
									std::string text = tile.object->get_description();

									const std::string effects_string = tile.object->get_use_effects()->get_effects_string(character, ctx);
									if (!text.empty()) {
										text += "\n\n";
									}
									text += effects_string;

									engine_interface::get()->add_combat_notification(std::format("{} {}", tile.object->get_object_type()->get_name(), tile.object->get_object_type()->get_usage_adjective()), nullptr, text);
								}

								tile.object->get_use_effects()->do_effects(character, ctx);
							}

							this->remove_object(tile.object);
							attacked = true;
						} else {
							if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
								const portrait *war_minister_portrait = character->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();

								engine_interface::get()->add_combat_notification(std::format("Cannot Use {}", tile.object->get_object_type()->get_name()), war_minister_portrait, std::format("Your Excellency, the {} can only be used once all enemies have been defeated.", string::lowered(tile.object->get_object_type()->get_name())));
							}
						}
					}
				} else if (this->can_current_unit_move_to(target_pos)) {
					character_info->change_remaining_movement(-distance);
					co_await this->move_character_to(character, target_pos);

					if (this->can_current_unit_retreat_at(target_pos)) {
						party->remove_character(character);
						this->remove_character_info(character);
						break;
					}
				}
			}
		}
	}

	if (this->get_current_unit() != nullptr) {
		this->set_current_unit(nullptr);
	}

	co_return experience_award;
}

const character *combat::choose_enemy(const character *character, const std::vector<const metternich::character *> &enemies) const
{
	std::vector<const metternich::character *> potential_enemies;
	int best_distance = std::numeric_limits<int>::max();

	const combat_character_info *character_info = this->get_character_info(character);
	const QPoint tile_pos = character_info->get_tile_pos();

	for (const metternich::character *enemy : enemies) {
		const combat_character_info *enemy_info = this->get_character_info(enemy);
		const QPoint enemy_tile_pos = enemy_info->get_tile_pos();
		const int distance = point::distance_to(tile_pos, enemy_tile_pos);
		if (distance < best_distance) {
			potential_enemies.clear();
			best_distance = distance;
		}

		if (distance <= best_distance) {
			potential_enemies.push_back(enemy);
		}
	}

	if (potential_enemies.empty()) {
		return nullptr;
	}

	return vector::get_random(potential_enemies);
}

const combat_object *combat::choose_target_object(const character *character) const
{
	std::vector<const combat_object *> potential_objects;
	int best_distance = std::numeric_limits<int>::max();

	const combat_character_info *character_info = this->get_character_info(character);
	const QPoint tile_pos = character_info->get_tile_pos();

	for (const qunique_ptr<combat_object> &object : this->objects) {
		const QPoint object_tile_pos = object->get_tile_pos();
		const int distance = point::distance_to(tile_pos, object_tile_pos);
		if (distance < best_distance) {
			potential_objects.clear();
			best_distance = distance;
		}

		if (distance <= best_distance) {
			potential_objects.push_back(object.get());
		}
	}

	if (potential_objects.empty()) {
		return nullptr;
	}

	return vector::get_random(potential_objects);
}

bool combat::do_to_hit_check(const character *character, const metternich::character *enemy, const int to_hit_modifier) const
{
	static constexpr dice to_hit_dice(1, 20);
	const int to_hit = 20 - character->get_game_data()->get_to_hit_bonus() - to_hit_modifier;
	const int to_hit_result = to_hit - random::get()->roll_dice(to_hit_dice);

	const int armor_class_bonus = enemy->get_game_data()->get_armor_class_bonus() + enemy->get_game_data()->get_species_armor_class_bonus(character->get_species());
	const int armor_class = 10 - armor_class_bonus;
	if (to_hit_result > armor_class) {
		return false;
	}

	return true;
}

int64_t combat::do_character_attack(const character *character, const metternich::character *enemy, party *enemy_party, const int to_hit_modifier)
{
	const bool hit = this->do_to_hit_check(character, enemy, to_hit_modifier);
	if (hit) {
		return 0;
	}

	const int damage = random::get()->roll_dice(character->get_game_data()->get_damage_dice()) + character->get_game_data()->get_damage_bonus();
	enemy->get_game_data()->change_hit_points(-damage);

	if (enemy->get_game_data()->is_dead()) {
		const int64_t experience_award = enemy->get_game_data()->get_experience_award();
		this->on_character_killed(enemy, enemy_party, character);
		return experience_award;
	}

	return 0;
}

QCoro::Task<int64_t> combat::do_character_spellcast(const character *caster, const spell *spell, const metternich::character *target, party *target_party, const int to_hit_modifier)
{
	assert_throw(caster != nullptr);
	assert_throw(caster->get_game_data()->can_cast_spell(spell));

	caster->get_game_data()->change_mana(-spell->get_mana_cost());

	const bool hit = !spell->requires_to_hit_check() || this->do_to_hit_check(caster, target, to_hit_modifier);

	if (this->scope == game::get()->get_player_country()) {
		if (spell->get_sound() != nullptr) {
			co_await spell->get_sound()->play_coro(std::chrono::milliseconds(100));
		}
	}

	if (!hit) {
		co_return 0;
	}

	if (spell->get_target_effects() != nullptr) {
		context ctx = this->ctx;
		ctx.root_scope = target;
		ctx.source_scope = caster;
		spell->get_target_effects()->do_effects(target, ctx);
	}

	if (target->get_game_data()->is_dead()) {
		if (spell->get_target() == spell_target::enemy) {
			const int64_t experience_award = target->get_game_data()->get_experience_award();
			this->on_character_killed(target, target_party, caster);
			co_return experience_award;
		} else {
			this->on_character_died(target, target_party);
		}
	}

	co_return 0;
}

void combat::on_character_killed(const character *dead_character, party *dead_character_party, const metternich::character *killer)
{
	if (killer->get_game_data()->get_domain() != nullptr) {
		const combat_character_info *dead_character_info = this->get_character_info(dead_character);
		assert_throw(dead_character_info != nullptr);
		if (dead_character_info->get_kill_effects() != nullptr) {
			context ctx = this->ctx;
			ctx.root_scope = killer->get_game_data()->get_domain();

			if (killer->get_game_data()->get_domain() == game::get()->get_player_country()) {
				const portrait *war_minister_portrait = killer->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();
				const std::string effects_string = dead_character_info->get_kill_effects()->get_effects_string(killer->get_game_data()->get_domain(), ctx);

				engine_interface::get()->add_combat_notification(std::format("{} Killed", dead_character->is_temporary() && dead_character->get_monster_type() != nullptr ? dead_character->get_monster_type()->get_name() : dead_character->get_game_data()->get_full_name()), war_minister_portrait, effects_string);
			}

			dead_character_info->get_kill_effects()->do_effects(killer->get_game_data()->get_domain(), ctx);
		}
	}

	this->on_character_died(dead_character, dead_character_party);
}

void combat::on_character_died(const character *dead_character, party *dead_character_party)
{
	if (this->scope == game::get()->get_player_country()) {
		//FIXME: play character death sound
	}

	dead_character_party->remove_character(dead_character);
	this->remove_character_info(dead_character);
}

void combat::process_result()
{
	const bool success = this->attacking_party->get_domain() == this->scope ? this->result.attacker_victory : !this->result.attacker_victory;

	context ctx = this->ctx;
	ctx.in_combat = false;

	if (this->scope == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->scope->get_government()->get_war_minister_portrait();

		if (success) {
			std::string effects_string = std::format("Experience: {}", number::to_signed_string(this->result.experience_award));
			if (this->victory_effects != nullptr) {
				const std::string victory_effects_string = this->victory_effects->get_effects_string(this->scope, ctx);
				effects_string += "\n" + victory_effects_string;
			}

			engine_interface::get()->add_combat_notification("Victory!", war_minister_portrait, std::format("You have won a combat!\n\n{}", effects_string));
		} else {
			std::string effects_string;
			if (this->defeat_effects != nullptr) {
				const std::string defeat_effects_string = this->defeat_effects->get_effects_string(this->scope, ctx);
				effects_string += "\n" + defeat_effects_string;
			}

			engine_interface::get()->add_combat_notification("Defeat!", war_minister_portrait, std::format("You have lost a combat!{}", !effects_string.empty() ? ("\n\n" + effects_string) : ""));
		}
	}

	if (success) {
		if (this->victory_effects != nullptr) {
			this->victory_effects->do_effects(this->scope, ctx);
		}
	} else {
		if (this->defeat_effects != nullptr) {
			this->defeat_effects->do_effects(this->scope, ctx);
		}
	}
}

combat_tile &combat::get_tile(const QPoint &tile_pos)
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
}

const combat_tile &combat::get_tile(const QPoint &tile_pos) const
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
}

std::string combat::get_tile_text(const QPoint &tile_pos) const
{
	std::string text = combat_base::get_tile_text(tile_pos);

	const combat_tile &tile = this->get_tile(tile_pos);
	if (tile.character != nullptr) {
		const character *character = tile.character;
		const character_game_data *character_game_data = character->get_game_data();
		const std::string type_name = character->get_monster_type() != nullptr ? character->get_monster_type()->get_name() : (character_game_data->get_character_class() != nullptr ? (character_game_data->get_character_class()->get_name() + " " + std::to_string(character_game_data->get_level())) : character->get_species()->get_name());
		const std::string full_name = character_game_data->get_full_name();
		text += " " + (!full_name.empty() ? (full_name + " (" + type_name + ")") : type_name);

		for (const auto &[status_effect, rounds] : character_game_data->get_status_effect_rounds()) {
			text += std::format(" ({})", status_effect->get_adjective());
		}
	} else if (tile.object != nullptr) {
		const combat_object *object = tile.object;
		text += " " + object->get_object_type()->get_name();

		if (object->get_trap() != nullptr && object->get_trap_found()) {
			text += " (Trap: " + object->get_trap()->get_name();
			if (this->get_current_unit() != nullptr) {
				text += " - " + std::to_string(object->get_disarm_chance(static_cast<const combat_character_info *>(this->get_current_unit())->get_character())) + "% Disarm Chance";
			}
			text += ")";
		}
	}

	return text;
}

combat_unit_info_base *combat::get_tile_unit(const QPoint &tile_pos) const
{
	const combat_tile &tile = this->get_tile(tile_pos);
	if (tile.character != nullptr) {
		return this->get_character_info(tile.character);
	}

	return nullptr;
}

bool combat::is_attacker_defeated() const
{
	return this->attacking_party->get_characters().empty();
}

bool combat::is_defender_defeated() const
{
	return this->defending_party->get_characters().empty();
}

[[nodiscard]]
QCoro::Task<void> combat::move_character_to(const character *character, const QPoint tile_pos)
{
	combat_character_info *character_info = this->get_character_info(character);
	const QPoint old_tile_pos = character_info->get_tile_pos();

	if (this->scope == game::get()->get_player_country()) {
		static constexpr int milliseconds_per_tile = 200;

		const int distance = point::distance_to(old_tile_pos, tile_pos);
		const int animation_ms = distance * milliseconds_per_tile;
		const QPoint pixel_difference = (tile_pos - old_tile_pos) * size::to_point(defines::get()->get_scaled_tile_size());

		static constexpr int timer_interval_ms = 10;
		QTimer timer;
		timer.setInterval(std::chrono::milliseconds(timer_interval_ms));
		timer.start();

		for (int ms = 0; ms < animation_ms; ms += timer_interval_ms) {
			co_await timer;

			const QPoint pixel_offset = pixel_difference * ms / animation_ms;
			character_info->set_pixel_offset(pixel_offset);
		}
	}

	combat_tile &tile = this->get_tile(tile_pos);
	assert_throw(!tile.is_occupied());

	combat_tile &old_tile = this->get_tile(old_tile_pos);
	assert_throw(old_tile.character == character);
	old_tile.character = nullptr;

	character_info->set_tile_pos(tile_pos);
	tile.character = character;

	//check for traps in adjacent tiles
	point::for_each_adjacent(tile_pos, [&](const QPoint &adjacent_pos) {
		if (!this->get_map_rect().contains(adjacent_pos)) {
			return;
		}

		const combat_tile &adjacent_tile = this->get_tile(adjacent_pos);
		if (adjacent_tile.object == nullptr) {
			return;
		}

		if (!this->can_character_use_object(character, adjacent_tile.object)) {
			return;
		}

		if (adjacent_tile.object->get_trap() == nullptr) {
			return;
		}

		if (!adjacent_tile.object->get_trap_found() && skill::get_find_traps_skill() != nullptr) {
			const bool found = character->get_game_data()->do_skill_check(skill::get_find_traps_skill(), adjacent_tile.object->get_trap()->get_find_modifier(), this->get_location());
			if (found) {
				adjacent_tile.object->set_trap_found(found);
			}
		}
	});

	emit tile_unit_changed(old_tile_pos);
	emit tile_unit_changed(tile_pos);
}

bool combat::is_current_unit_in_enemy_range_at(const QPoint &tile_pos) const
{
	if (this->get_current_unit() == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const party *enemy_party = this->get_current_unit()->is_defender() ? this->attacking_party : this->defending_party;

	for (const character *enemy : enemy_party->get_characters()) {
		const combat_character_info *enemy_info = this->get_character_info(enemy);
		const QPoint enemy_tile_pos = enemy_info->get_tile_pos();
		const int distance = point::distance_to(enemy_tile_pos, tile_pos);

		if (distance <= enemy_info->get_range()) {
			return true;
		}
	}

	return false;
}

bool combat::can_character_use_object(const character *character, const combat_object *object) const
{
	assert_throw(character != nullptr);
	assert_throw(object != nullptr);

	if (!vector::contains(this->attacking_party->get_characters(), character)) {
		return false;
	}

	if (!this->defending_party->get_characters().empty()) {
		return false;
	}

	return true;
}

bool combat::can_current_character_use_object(const combat_object *object) const
{
	if (this->get_current_unit() == nullptr) {
		return false;
	}

	return this->can_character_use_object(static_cast<const combat_character_info *>(this->get_current_unit())->get_character(), object);
}

const site *combat::get_location() const
{
	assert_throw(ctx.dungeon_site != nullptr || this->scope != nullptr);

	if (ctx.dungeon_site != nullptr) {
		return ctx.dungeon_site;
	}

	return this->scope->get_game_data()->get_capital();
}

combat_tile::combat_tile(const terrain_type *base_terrain, const terrain_type *terrain)
	: combat_tile_base(base_terrain, terrain)
{
}

combat_character_info::combat_character_info(const metternich::character *character, const bool defender)
	: combat_unit_info_base(defender), character(character)
{
	connect(character->get_game_data(), &character_game_data::icon_changed, this, &combat_character_info::icon_changed);
	connect(character->get_game_data(), &character_game_data::hit_points_changed, this, &combat_character_info::hit_points_changed);
	connect(character->get_game_data(), &character_game_data::max_hit_points_changed, this, &combat_character_info::max_hit_points_changed);
}

const icon *combat_character_info::get_icon() const
{
	return this->get_character()->get_game_data()->get_icon();
}

int combat_character_info::get_hit_points() const
{
	return this->get_character()->get_game_data()->get_hit_points();
}

int combat_character_info::get_max_hit_points() const
{
	return this->get_character()->get_game_data()->get_max_hit_points();
}

int combat_character_info::get_range() const
{
	return this->get_character()->get_game_data()->get_range();
}

bool combat_character_info::is_player_unit() const
{
	return this->get_character()->get_game_data()->get_domain() == game::get()->get_player_country();
}

bool combat_character_info::is_player_enemy() const
{
	return this->get_character()->get_game_data()->get_domain() != game::get()->get_player_country();
}

int combat_object::get_disarm_chance(const metternich::character *character) const
{
	assert_throw(character != nullptr);
	assert_throw(this->get_trap() != nullptr);

	if (skill::get_disarm_traps_skill() == nullptr) {
		return 0;
	}

	return character->get_game_data()->get_skill_check_chance(skill::get_disarm_traps_skill(), this->get_trap()->get_disarm_modifier(), this->combat->get_location());
}

}
