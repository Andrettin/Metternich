#include "metternich.h"

#include "game/combat.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/party.h"
#include "database/defines.h"
#include "domain/country_government.h"
#include "domain/domain.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
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
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

combat::combat(party *attacking_party, party *defending_party)
	: attacking_party(attacking_party), defending_party(defending_party)
{
	this->map_rect = QRect(QPoint(0, 0), QSize(combat::map_width, combat::map_height));

	this->base_terrain = defines::get()->get_default_base_terrain();
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

void combat::initialize()
{
	const size_t tile_count = static_cast<size_t>(this->get_map_width() * this->get_map_height());
	this->tiles.reserve(tile_count);

	for (size_t i = 0; i < tile_count; ++i) {
		this->tiles.emplace_back(this->base_terrain, this->base_terrain);
	}

	const QPoint attacker_start_pos(1, this->get_map_height() / 2);
	const QPoint defender_start_pos(this->get_map_width() - 2, this->get_map_height() / 2);

	this->deploy_characters(this->attacking_party->get_characters(), attacker_start_pos, false);
	this->deploy_characters(this->defending_party->get_characters(), defender_start_pos, true);
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
			if (tile.character != nullptr) {
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

	while (!this->attacking_party->get_characters().empty() && !this->defending_party->get_characters().empty()) {
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

	assert_throw(!enemy_party->get_characters().empty());

	int64_t experience_award = 0;

	for (const character *character : party->get_characters()) {
		if (enemy_party->get_characters().empty()) {
			break;
		}

		if (party->get_domain() == game::get()->get_player_country()) {
			bool attacked = false;
			combat_character_info *character_info = this->get_character_info(character);
			character_info->set_remaining_movement(character->get_game_data()->get_combat_movement());

			this->current_character = character;

			while (!attacked && character_info->get_remaining_movement() > 0) {
				emit movable_tiles_changed();

				this->target_promise = std::make_unique<QPromise<QPoint>>();
				const QFuture<QPoint> target_future = this->target_promise->future();
				this->target_promise->start();

				const QPoint current_tile_pos = character_info->get_tile_pos();

				const QPoint target_pos = co_await target_future;
				const combat_tile &tile = this->get_tile(target_pos);
				const int distance = point::distance_to(current_tile_pos, target_pos);

				if (tile.character != nullptr) {
					if (distance <= character->get_game_data()->get_range() && vector::contains(enemy_party->get_characters(), tile.character)) {
						experience_award += this->do_character_attack(character, tile.character, enemy_party, to_hit_modifier);
						attacked = true;
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
		} else {
			const metternich::character *chosen_enemy = vector::get_random(enemy_party->get_characters());
			experience_award += this->do_character_attack(character, chosen_enemy, enemy_party, to_hit_modifier);
		}
	}

	if (this->current_character != nullptr) {
		this->current_character = nullptr;
		emit movable_tiles_changed();
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

const combat_tile &combat::get_tile(const QPoint &tile_pos) const
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
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
	assert_throw(tile.character == nullptr);

	combat_tile &old_tile = this->get_tile(old_tile_pos);
	assert_throw(old_tile.character == character);
	old_tile.character = nullptr;

	character_info->set_tile_pos(tile_pos);
	tile.character = character;

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

	const combat_character_info *character_info = this->get_character_info(this->current_character);
	const QPoint current_tile_pos = character_info->get_tile_pos();

	const int distance = point::distance_to(current_tile_pos, tile_pos);

	if (distance > character_info->get_remaining_movement()) {
		return false;
	}

	const combat_tile &tile = this->get_tile(tile_pos);
	if (tile.character != nullptr) {
		return false;
	}

	return true;
}

bool combat::can_current_character_retreat_at(const QPoint &tile_pos) const
{
	if (this->current_character == nullptr) {
		return false;
	}

	const combat_character_info *character_info = this->get_character_info(this->current_character);
	if (character_info->is_defender()) {
		return this->defender_retreat_allowed && this->is_tile_defender_escape(tile_pos);
	} else {
		return this->attacker_retreat_allowed && this->is_tile_attacker_escape(tile_pos);
	}
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

}
