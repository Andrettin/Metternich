#include "metternich.h"

#include "game/combat.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/monster_type.h"
#include "character/party.h"
#include "character/skill.h"
#include "database/defines.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "item/object_type.h"
#include "item/trap_type.h"
#include "map/terrain_type.h"
#include "script/effect/effect_list.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/dice.h"
#include "util/map_util.h"
#include "util/number_util.h"
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
	QSize min_map_size = (QGuiApplication::primaryScreen()->size() / defines::get()->get_scaled_tile_size()) - QSize(1, 0);

	int max_range = 0;
	for (const character *character : attacking_party->get_characters()) {
		max_range = std::max(max_range, character->get_game_data()->get_range());
	}
	for (const character *character : defending_party->get_characters()) {
		max_range = std::max(max_range, character->get_game_data()->get_range());
	}

	min_map_size.setWidth(std::max(min_map_size.width(), max_range + 4));

	this->map_rect = QRect(QPoint(0, 0), QSize(std::max(map_size.width(), min_map_size.width()), std::max(map_size.height(), min_map_size.height())));

	this->base_terrain = defines::get()->get_default_base_terrain();

	connect(this, &combat::current_character_changed, this, &combat::movable_tiles_changed);
}

combat::~combat()
{
}

void combat::set_base_terrain(const terrain_type *terrain)
{
	assert_throw(terrain != nullptr);

	this->base_terrain = terrain;
}

void combat::set_generated_party(std::unique_ptr<party> &&generated_party)
{
	this->generated_party = std::move(generated_party);
}

QVariantList combat::get_character_infos_qvariant_list() const
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

	emit tile_character_changed(tile_pos);
	emit character_infos_changed();
}

QVariantList combat::get_objects_qvariant_list() const
{
	return container::to_qvariant_list(this->objects);
}

void combat::add_object(const object_type *object_type, const effect_list<const character> *use_effects, const trap_type *trap)
{
	auto object = make_qunique<combat_object>(object_type, use_effects, trap);
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
		this->tiles.emplace_back(this->base_terrain, this->base_terrain);
	}

	const QPoint attacker_start_pos(1, this->get_map_height() / 2);
	const QPoint defender_start_pos(this->get_map_width() - 2, this->get_map_height() / 2);

	this->deploy_objects(defender_start_pos);
	this->deploy_characters(this->attacking_party->get_characters(), attacker_start_pos, false);
	this->deploy_characters(this->defending_party->get_characters(), defender_start_pos, true);
}

void combat::deploy_objects(const QPoint &start_pos)
{
	vector::shuffle(this->objects);

	std::vector<QPoint> tiles_to_check{ start_pos };
	size_t last_check_index = 0;

	for (const qunique_ptr<combat_object> &object : this->objects) {
		for (size_t i = last_check_index; i < tiles_to_check.size(); ++i) {
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

			last_check_index = i;
			object->set_tile_pos(tile_pos);
			tile.object = object.get();
			break;
		}
	}
}

void combat::deploy_characters(std::vector<const character *> characters, const QPoint &start_pos, const bool defenders)
{
	std::sort(characters.begin(), characters.end(), [](const character *lhs, const character *rhs) {
		if (lhs->get_game_data()->get_movement() != rhs->get_game_data()->get_movement()) {
			return lhs->get_game_data()->get_movement() < rhs->get_game_data()->get_movement();
		}

		if (lhs->get_game_data()->get_range() != rhs->get_game_data()->get_range()) {
			return lhs->get_game_data()->get_range() > rhs->get_game_data()->get_range();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	std::vector<QPoint> tiles_to_check{ start_pos };
	size_t last_check_index = 0;

	for (const character *character : characters) {
		for (size_t i = last_check_index; i < tiles_to_check.size(); ++i) {
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

			last_check_index = i;
			tile.character = character;
			auto character_info = make_qunique<combat_character_info>(character, tile_pos, defenders);
			this->character_infos[character] = std::move(character_info);
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

	if (game::get()->get_current_combat() == this) {
		game::get()->set_current_combat(nullptr);
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

		this->current_character = character;
		emit current_character_changed();

		while (!attacked && character_info->get_remaining_movement() > 0) {
			const QPoint current_tile_pos = character_info->get_tile_pos();

			QPoint target_pos(-1, -1);

			if (party->get_domain() == game::get()->get_player_country() && !this->is_autoplay_enabled()) {
				emit movable_tiles_changed();

				this->target_promise = std::make_unique<QPromise<QPoint>>();
				const QFuture<QPoint> target_future = this->target_promise->future();
				this->target_promise->start();

				target_pos = co_await target_future;

				if (this->is_autoplay_enabled()) {
					continue;
				}
			} else {
				QPoint chosen_target_tile_pos(-1, -1);

				if (!enemy_party->get_characters().empty()) {
					const metternich::character *chosen_enemy = vector::get_random(enemy_party->get_characters());
					const combat_character_info *chosen_enemy_info = this->get_character_info(chosen_enemy);
					chosen_target_tile_pos = chosen_enemy_info->get_tile_pos();
				} else {
					assert_throw(!this->objects.empty());
					const qunique_ptr<combat_object> &chosen_object = vector::get_random(this->objects);
					chosen_target_tile_pos = chosen_object->get_tile_pos();
				}

				assert_throw(chosen_target_tile_pos != QPoint(-1, -1));

				const int distance_to_target = point::distance_to(current_tile_pos, chosen_target_tile_pos);
				if (distance_to_target <= character->get_game_data()->get_range()) {
					target_pos = chosen_target_tile_pos;
				} else {
					const int current_square_distance = point::square_distance_to(current_tile_pos, chosen_target_tile_pos);
					int best_square_distance = std::numeric_limits<int>::max();
					std::vector<QPoint> potential_tiles;

					point::for_each_adjacent(current_tile_pos, [&](const QPoint &adjacent_pos) {
						if (!this->get_map_rect().contains(adjacent_pos)) {
							return;
						}

						if (!this->can_current_character_move_to(adjacent_pos)) {
							return;
						}

						if (this->can_current_character_retreat_at(adjacent_pos)) {
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

			if (tile.character != nullptr) {
				if (distance <= character->get_game_data()->get_range() && vector::contains(enemy_party->get_characters(), tile.character)) {
					experience_award += this->do_character_attack(character, tile.character, enemy_party, to_hit_modifier);
					attacked = true;
				}
			} else if (tile.object != nullptr) {
				//can only use objects (e.g. chests) if the enemy has been wiped out
				if (distance <= 1 && this->can_current_character_use_object(tile.object)) {
					if (tile.object->get_trap() != nullptr) {
						bool disarmed = false;
						if (tile.object->get_trap_found() && skill::get_disarm_traps_skill() != nullptr) {
							disarmed = character->get_game_data()->do_skill_check(skill::get_disarm_traps_skill(), tile.object->get_trap()->get_disarm_modifier());
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
						party->remove_character(character);
						this->remove_character_info(character);
						break;
					}

					if (tile.object->get_use_effects() != nullptr) {
						context ctx = this->ctx;
						ctx.root_scope = character;

						if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
							const portrait *war_minister_portrait = character->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();
							const std::string effects_string = tile.object->get_use_effects()->get_effects_string(character, ctx);

							engine_interface::get()->add_combat_notification(std::format("{} Used", tile.object->get_object_type()->get_name()), war_minister_portrait, effects_string);
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
			} else if (this->can_current_character_move_to(target_pos)) {
				character_info->change_remaining_movement(-distance);
				co_await this->move_character_to(character, target_pos);

				if (this->can_current_character_retreat_at(target_pos)) {
					party->remove_character(character);
					this->remove_character_info(character);
					break;
				}
			}
		}
	}

	if (this->current_character != nullptr) {
		this->current_character = nullptr;
		emit current_character_changed();
	}

	co_return experience_award;
}

int64_t combat::do_character_attack(const character *character, const metternich::character *enemy, party *enemy_party, const int to_hit_modifier)
{
	static constexpr dice to_hit_dice(1, 20);
	const int to_hit = 20 - character->get_game_data()->get_to_hit_bonus() - to_hit_modifier;
	const int to_hit_result = to_hit - random::get()->roll_dice(to_hit_dice);

	const int armor_class_bonus = enemy->get_game_data()->get_armor_class_bonus() + enemy->get_game_data()->get_species_armor_class_bonus(character->get_species());
	const int armor_class = 10 - armor_class_bonus;
	if (to_hit_result > armor_class) {
		return 0;
	}

	const int damage = random::get()->roll_dice(character->get_game_data()->get_damage_dice()) + character->get_game_data()->get_damage_bonus();
	enemy->get_game_data()->change_hit_points(-damage);

	if (enemy->get_game_data()->is_dead()) {
		if (character->get_game_data()->get_domain() != nullptr) {
			const auto find_iterator = this->character_kill_effects.find(enemy);
			if (find_iterator != this->character_kill_effects.end()) {
				context ctx = this->ctx;
				ctx.root_scope = character->get_game_data()->get_domain();

				if (character->get_game_data()->get_domain() == game::get()->get_player_country()) {
					const portrait *war_minister_portrait = character->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();
					const std::string effects_string = find_iterator->second->get_effects_string(character->get_game_data()->get_domain(), ctx);

					engine_interface::get()->add_combat_notification(std::format("{} Killed", enemy->is_temporary() && enemy->get_monster_type() != nullptr ? enemy->get_monster_type()->get_name() : enemy->get_full_name()), war_minister_portrait, effects_string);
				}

				find_iterator->second->do_effects(character->get_game_data()->get_domain(), ctx);
			}
		}

		const int64_t experience_award = enemy->get_game_data()->get_experience_award();
		enemy_party->remove_character(enemy);
		this->remove_character_info(enemy);

		return experience_award;
	}

	return 0;
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

bool combat::is_tile_attacker_escape(const QPoint &tile_pos) const
{
	//can only retreat if the enemy is still present; this is to prevent a retreat from happening while opening chests, leading to potentially opening the same chest multiple times
	return tile_pos.x() == 0 && !this->defending_party->get_characters().empty();
}

bool combat::is_tile_defender_escape(const QPoint &tile_pos) const
{
	return tile_pos.x() == (this->get_map_width() - 1) && !this->attacking_party->get_characters().empty();
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
			const bool found = character->get_game_data()->do_skill_check(skill::get_find_traps_skill(), adjacent_tile.object->get_trap()->get_find_modifier());
			if (found) {
				adjacent_tile.object->set_trap_found(found);
			}
		}
	});

	emit tile_character_changed(old_tile_pos);
	emit tile_character_changed(tile_pos);
}

void combat::set_target(const QPoint &tile_pos)
{
	if (this->target_promise == nullptr) {
		return;
	}

	this->target_promise->addResult(tile_pos);
	this->target_promise->finish();
}

bool combat::can_current_character_move_to(const QPoint &tile_pos) const
{
	if (this->current_character == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const combat_character_info *character_info = this->get_character_info(this->current_character);
	const QPoint current_tile_pos = character_info->get_tile_pos();

	const int distance = point::distance_to(current_tile_pos, tile_pos);

	if (distance > character_info->get_remaining_movement()) {
		return false;
	}

	const combat_tile &tile = this->get_tile(tile_pos);
	if (tile.is_occupied()) {
		return false;
	}

	return true;
}

bool combat::can_current_character_retreat_at(const QPoint &tile_pos) const
{
	if (this->current_character == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const combat_character_info *character_info = this->get_character_info(this->current_character);
	if (character_info->is_defender()) {
		return this->defender_retreat_allowed && this->is_tile_defender_escape(tile_pos);
	} else {
		return this->attacker_retreat_allowed && this->is_tile_attacker_escape(tile_pos);
	}
}

bool combat::is_current_character_in_enemy_range_at(const QPoint &tile_pos) const
{
	if (this->current_character == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const combat_character_info *character_info = this->get_character_info(this->current_character);
	const party *enemy_party = character_info->is_defender() ? this->attacking_party : this->defending_party;

	for (const character *enemy : enemy_party->get_characters()) {
		const combat_character_info *enemy_info = this->get_character_info(enemy);
		const QPoint enemy_tile_pos = enemy_info->get_tile_pos();
		const int distance = point::distance_to(enemy_tile_pos, tile_pos);

		if (distance <= enemy->get_game_data()->get_range()) {
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
	if (this->current_character == nullptr) {
		return false;
	}

	return this->can_character_use_object(this->current_character, object);
}

combat_tile::combat_tile(const terrain_type *base_terrain, const terrain_type *terrain)
{
	this->terrain = terrain;

	if (!base_terrain->get_subtiles().empty()) {
		std::array<const std::vector<int> *, 4> terrain_subtiles{};

		for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
			terrain_subtiles[i] = &base_terrain->get_subtiles();
		}

		for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
			const short terrain_subtile = static_cast<short>(vector::get_random(*terrain_subtiles[i]));

			this->base_subtile_frames[i] = terrain_subtile;
		}
	} else {
		this->base_tile_frame = static_cast<short>(vector::get_random(base_terrain->get_tiles()));
	}
}

int combat_object::get_disarm_chance(const metternich::character *character) const
{
	assert_throw(character != nullptr);
	assert_throw(this->get_trap() != nullptr);

	if (skill::get_disarm_traps_skill() == nullptr) {
		return 0;
	}

	return character->get_game_data()->get_skill_check_chance(skill::get_disarm_traps_skill(), this->get_trap()->get_disarm_modifier());
}

}
