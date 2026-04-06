#include "metternich.h"

#include "game/battle.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "engine_interface.h"
#include "game/attack_result.h"
#include "game/battle_resolution_table.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "script/effect/effect_list.h"
#include "sound/sound.h"
#include "spell/spell.h"
#include "spell/spell_target.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
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

battle::battle(army *attacking_army, army *defending_army, const QSize &map_size)
	: attacking_army(attacking_army), defending_army(defending_army)
{
	this->set_map_size(map_size);

	for (const military_unit *unit : this->attacking_army->get_military_units()) {
		auto unit_info = make_qunique<battle_unit_info>(unit, false);
		this->unit_infos[unit] = std::move(unit_info);
	}
	for (const military_unit *unit : this->defending_army->get_military_units()) {
		auto unit_info = make_qunique<battle_unit_info>(unit, true);
		this->unit_infos[unit] = std::move(unit_info);
	}
}

battle::~battle()
{
}

int battle::get_max_range_of_units() const
{
	int max_range = 0;

	for (const military_unit *unit : this->attacking_army->get_military_units()) {
		max_range = std::max(max_range, unit->get_stat(military_unit_stat::range).to_int());
	}
	for (const military_unit *unit : this->defending_army->get_military_units()) {
		max_range = std::max(max_range, unit->get_stat(military_unit_stat::range).to_int());
	}

	return max_range;
}

spell_target battle::get_spell_target(const spell *spell) const
{
	return spell->get_battle_target();
}

int battle::get_spell_range(const spell *spell) const
{
	return spell->get_battle_range();
}

QVariantList battle::get_unit_infos_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->unit_infos);
}

battle_unit_info *battle::get_unit_info(const military_unit *unit) const
{
	const auto find_iterator = this->unit_infos.find(unit);
	assert_throw(find_iterator != this->unit_infos.end());
	return find_iterator->second.get();
}

void battle::remove_unit_info(const military_unit *unit)
{
	const battle_unit_info *unit_info = this->get_unit_info(unit);
	const QPoint tile_pos = unit_info->get_tile_pos();
	this->get_tile(tile_pos).unit = nullptr;
	this->unit_infos.erase(unit);

	emit tile_unit_changed(tile_pos);
	emit unit_infos_changed();
}

void battle::initialize()
{
	const size_t tile_count = static_cast<size_t>(this->get_map_width() * this->get_map_height());
	this->tiles.reserve(tile_count);

	for (size_t i = 0; i < tile_count; ++i) {
		this->tiles.emplace_back(this->get_base_terrain(), this->get_base_terrain());
	}

	this->deploy_units(this->attacking_army->get_military_units(), false);
	this->deploy_units(this->defending_army->get_military_units(), true);
}

void battle::deploy_units(std::vector<military_unit *> units, const bool defenders)
{
	const QPoint left_start_pos(1, (this->get_map_height() - 1) / 2);
	const QPoint right_start_pos(this->get_map_width() - 2, (this->get_map_height() - 1) / 2);
	const QPoint center_start_pos((this->get_map_width() - 1) / 2, (this->get_map_height() - 1) / 2);

	std::sort(units.begin(), units.end(), [](const military_unit *lhs, const military_unit *rhs) {
		if (lhs->get_battle_movement() != rhs->get_battle_movement()) {
			return lhs->get_battle_movement() < rhs->get_battle_movement();
		}

		if (lhs->get_stat(military_unit_stat::range) != rhs->get_stat(military_unit_stat::range)) {
			return lhs->get_stat(military_unit_stat::range) > rhs->get_stat(military_unit_stat::range);
		}

		return lhs < rhs;
	});

	for (military_unit *unit : units) {
		battle_unit_info *unit_info = this->get_unit_info(unit);
		assert_throw(unit_info != nullptr);

		QPoint start_tile_pos;

		switch (unit_info->get_placement()) {
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

		start_tile_pos += unit_info->get_placement_offset();

		std::vector<QPoint> tiles_to_check = { start_tile_pos };

		for (size_t i = 0; i < tiles_to_check.size(); ++i) {
			const QPoint &tile_pos = tiles_to_check.at(i);

			if (this->is_tile_attacker_escape(tile_pos) || this->is_tile_defender_escape(tile_pos)) {
				continue;
			}

			battle_tile &tile = this->get_tile(tile_pos);
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

			tile.unit = unit;
			unit_info->set_tile_pos(tile_pos);
			break;
		}
	}
}

QCoro::Task<void> battle::start_coro()
{
	this->get_promise()->start();

	while (!this->attacking_army->get_military_units().empty() && !this->defending_army->get_military_units().empty()) {
		co_await this->do_round();
	}

	this->result.attacker_victory = this->defending_army->get_military_units().empty();

	this->get_promise()->addResult(this->result.attacker_victory);
	this->get_promise()->finish();

	this->notify_result();

	emit finished();

	if (this->scope != game::get()->get_player_country()) {
		this->on_ended();
	}
}

QCoro::Task<void> battle::do_round()
{
	std::vector<military_unit *> all_units = this->attacking_army->get_military_units();
	vector::merge(all_units, this->defending_army->get_military_units());

	std::sort(all_units.begin(), all_units.end(), [](const military_unit *lhs, const military_unit *rhs) {
		if (lhs->get_battle_movement() != rhs->get_battle_movement()) {
			return lhs->get_battle_movement() > rhs->get_battle_movement();
		}

		return lhs < rhs;
	});

	std::vector<military_unit *> killed_units;
	for (military_unit *unit : all_units) {
		if (vector::contains(killed_units, unit)) {
			continue;
		}

		if (this->attacking_army->get_military_units().empty() || this->defending_army->get_military_units().empty()) {
			break;
		}

		co_await this->do_unit_round(unit, killed_units);
	}

	if (this->get_current_unit() != nullptr) {
		this->set_current_unit(nullptr);
	}
}

QCoro::Task<void> battle::do_unit_round(military_unit *unit, std::vector<military_unit *> &killed_units)
{
	bool attacked = false;
	battle_unit_info *unit_info = this->get_unit_info(unit);
	unit_info->set_remaining_movement(unit->get_battle_movement());

	this->set_current_unit(unit_info);

	army *army = unit_info->is_defender() ? this->defending_army : this->attacking_army;
	metternich::army *enemy_army = unit_info->is_defender() ? this->attacking_army : this->defending_army;

	while (!attacked && unit_info->get_remaining_movement() > 0) {
		const QPoint current_tile_pos = unit_info->get_tile_pos();

		QPoint target_pos(-1, -1);

		if (army->get_domain() == game::get()->get_player_country() && !this->is_autoplay_enabled()) {
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

			if (!enemy_army->get_military_units().empty()) {
				const metternich::military_unit *chosen_enemy = this->choose_enemy(unit, enemy_army->get_military_units());
				assert_throw(chosen_enemy != nullptr);
				const battle_unit_info *chosen_enemy_info = this->get_unit_info(chosen_enemy);
				chosen_target_tile_pos = chosen_enemy_info->get_tile_pos();
			}

			assert_throw(chosen_target_tile_pos != QPoint(-1, -1));

			const int distance_to_target = point::distance_to(current_tile_pos, chosen_target_tile_pos);
			if (distance_to_target <= unit_info->get_range()) {
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

		const battle_tile &tile = this->get_tile(target_pos);
		const int distance = point::distance_to(current_tile_pos, target_pos);

		if (this->get_current_spell() != nullptr) {
			if (tile.unit != nullptr) {
				if (this->get_current_spell()->get_battle_target() == spell_target::enemy && vector::contains(enemy_army->get_military_units(), tile.unit)) {
					if (distance <= this->get_current_spell()->get_battle_range()) {
						co_await this->do_unit_spellcast(unit, this->get_current_spell(), tile.unit, killed_units);
						attacked = true;
					}
				} else if (this->get_current_spell()->get_battle_target() == spell_target::ally && vector::contains(army->get_military_units(), tile.unit)) {
					if (distance <= this->get_current_spell()->get_battle_range()) {
						co_await this->do_unit_spellcast(unit, this->get_current_spell(), tile.unit, killed_units);
						attacked = true;
					}
				}
			}

			this->set_current_spell(nullptr);
		} else {
			if (tile.unit != nullptr) {
				if (distance <= unit_info->get_range() && vector::contains(enemy_army->get_military_units(), tile.unit)) {
					co_await this->do_unit_attack(unit, tile.unit, enemy_army, killed_units);
					attacked = true;
				}
			} else if (this->can_current_unit_move_to(target_pos)) {
				unit_info->change_remaining_movement(-distance);
				co_await this->move_unit_to(unit, target_pos);

				if (this->can_current_unit_retreat_at(target_pos)) {
					army->remove_military_unit(unit);
					this->remove_unit_info(unit);
					break;
				}
			}
		}
	}
}

const military_unit *battle::choose_enemy(const military_unit *unit, const std::vector<military_unit *> &enemies) const
{
	std::vector<const military_unit *> potential_enemies;
	int best_distance = std::numeric_limits<int>::max();

	const battle_unit_info *unit_info = this->get_unit_info(unit);
	const QPoint tile_pos = unit_info->get_tile_pos();

	for (const military_unit *enemy : enemies) {
		const battle_unit_info *enemy_info = this->get_unit_info(enemy);
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

QCoro::Task<void> battle::do_unit_attack(const military_unit *unit, military_unit *enemy, army *enemy_army, std::vector<military_unit *> &killed_units)
{
	const battle_unit_info *unit_info = this->get_unit_info(unit);
	const battle_unit_info *enemy_info = this->get_unit_info(enemy);

	const QPoint tile_pos = unit_info->get_tile_pos();
	const QPoint enemy_tile_pos = enemy_info->get_tile_pos();
	const int distance = point::distance_to(enemy_tile_pos, tile_pos);

	const bool moved = unit_info->get_remaining_movement() < unit->get_battle_movement();
	const bool ranged = distance > 1;

	int attack = 0;
	if (ranged) {
		attack = unit->get_effective_stat(military_unit_stat::missile).to_int();
	} else if (moved && unit->get_effective_stat(military_unit_stat::charge).to_int() > 0) {
		attack = unit->get_effective_stat(military_unit_stat::charge).to_int();
	} else {
		attack = unit->get_effective_stat(military_unit_stat::melee).to_int();
		if (enemy->get_type()->is_cavalry()) {
			attack += unit->get_effective_stat(military_unit_stat::melee_vs_mounted).to_int();
		}
	}

	int defense = enemy->get_effective_stat(military_unit_stat::defense).to_int();
	if (unit->get_type()->is_cavalry()) {
		defense += enemy->get_effective_stat(military_unit_stat::defense_vs_mounted).to_int();
	}

	const std::unique_ptr<battle_resolution_table> &battle_resolution_table = vector::get_random(defines::get()->get_battle_resolution_tables());

	const attack_result result = battle_resolution_table->get_result(unit->get_battle_resolution_type(), enemy->get_battle_resolution_type(), attack - defense);

	const military_unit_type *enemy_unit_type = enemy->get_type();

	switch (result) {
		case attack_result::miss:
		case attack_result::fall_back:
			break;
		case attack_result::hit:
		case attack_result::rout:
			enemy->change_hit_points(-1);
			break;
		case attack_result::destroy:
			enemy->change_hit_points(-enemy->get_hit_points());
			break;
	}

	if (this->scope == game::get()->get_player_country()) {
		if (!ranged && unit->get_type()->get_melee_attack_sound() != nullptr) {
			co_await unit->get_type()->get_melee_attack_sound()->play_coro(std::chrono::milliseconds(100));
		} else if (ranged && unit->get_type()->get_ranged_attack_sound() != nullptr) {
			co_await unit->get_type()->get_ranged_attack_sound()->play_coro(std::chrono::milliseconds(100));
		}
	}

	const bool enemy_dead = !vector::contains(enemy_army->get_military_units(), enemy);
	if (enemy_dead) {
		killed_units.push_back(enemy);
		this->remove_unit_info(enemy);

		if (this->scope == game::get()->get_player_country()) {
			if (enemy_unit_type != nullptr && enemy_unit_type->get_death_sound() != nullptr) {
				co_await enemy_unit_type->get_death_sound()->play_coro();
			}
		}
	}
}

QCoro::Task<void> battle::do_unit_spellcast(const military_unit *unit, const spell *spell, military_unit *target, std::vector<military_unit *> &killed_units)
{
	assert_throw(unit->get_character() != nullptr);
	assert_throw(unit->get_character()->get_game_data()->can_cast_spell(spell));

	const army *target_army = target->get_army();

	if (spell->get_battle_result() != attack_result::none) {
		const military_unit_type *target_unit_type = target->get_type();

		switch (spell->get_battle_result()) {
			case attack_result::miss:
			case attack_result::fall_back:
				break;
			case attack_result::hit:
			case attack_result::rout:
				target->change_hit_points(-1);
				break;
			case attack_result::destroy:
				target->change_hit_points(-target->get_hit_points());
				break;
		}

		unit->get_character()->get_game_data()->change_mana(-spell->get_mana_cost(unit->get_character()->get_game_data()->get_character_class()));

		if (this->scope == game::get()->get_player_country()) {
			if (spell->get_sound() != nullptr) {
				co_await spell->get_sound()->play_coro(std::chrono::milliseconds(100));
			}
		}

		const bool target_dead = !vector::contains(target_army->get_military_units(), target);
		if (target_dead) {
			killed_units.push_back(target);
			this->remove_unit_info(target);

			if (this->scope == game::get()->get_player_country()) {
				if (target_unit_type != nullptr && target_unit_type->get_death_sound() != nullptr) {
					co_await target_unit_type->get_death_sound()->play_coro();
				}
			}
		}
	}
}

void battle::notify_result()
{
	const bool success = this->attacking_army->get_domain() == this->scope ? this->result.attacker_victory : !this->result.attacker_victory;

	if (this->scope == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->scope->get_government()->get_war_minister_portrait();

		if (success) {
			engine_interface::get()->add_combat_notification("Victory!", war_minister_portrait, std::format("You have won a battle!"));
		} else {
			engine_interface::get()->add_combat_notification("Defeat!", war_minister_portrait, std::format("You have lost a battle!"));
		}
	}
}

void battle::process_result()
{
	const bool success = this->attacking_army->get_domain() == this->scope ? this->result.attacker_victory : !this->result.attacker_victory;

	context ctx = this->ctx;
	ctx.in_combat = false;

	if (success) {
		//FIXME: do post-battle effects, if necessary
	}
}

void battle::on_ended()
{
	this->process_result();

	this->clear();
}

battle_tile &battle::get_tile(const QPoint &tile_pos)
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
}

const battle_tile &battle::get_tile(const QPoint &tile_pos) const
{
	return this->tiles.at(point::to_index(tile_pos, this->get_map_width()));
}


std::string battle::get_tile_text(const QPoint &tile_pos) const
{
	std::string text = combat_base::get_tile_text(tile_pos);

	const battle_tile &tile = this->get_tile(tile_pos);
	if (tile.unit != nullptr) {
		const military_unit *unit = tile.unit;
		const std::string &type_name = unit->get_type()->get_name();
		const std::string &unit_name = unit->get_name();
		text += " " + (!unit_name.empty() ? (unit_name + " (" + type_name + ")") : type_name);
	}

	return text;
}

combat_unit_info_base *battle::get_tile_unit(const QPoint &tile_pos) const
{
	const battle_tile &tile = this->get_tile(tile_pos);
	if (tile.unit != nullptr) {
		return this->get_unit_info(tile.unit);
	}

	return nullptr;
}

bool battle::is_attacker_defeated() const
{
	return this->attacking_army->get_military_units().empty();
}

bool battle::is_defender_defeated() const
{
	return this->defending_army->get_military_units().empty();
}

[[nodiscard]]
QCoro::Task<void> battle::move_unit_to(military_unit *unit, const QPoint tile_pos)
{
	battle_unit_info *unit_info = this->get_unit_info(unit);
	const QPoint old_tile_pos = unit_info->get_tile_pos();

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
			unit_info->set_pixel_offset(pixel_offset);
		}
	}

	battle_tile &tile = this->get_tile(tile_pos);
	assert_throw(!tile.is_occupied());

	battle_tile &old_tile = this->get_tile(old_tile_pos);
	assert_throw(old_tile.unit == unit);
	old_tile.unit = nullptr;

	unit_info->set_tile_pos(tile_pos);
	tile.unit = unit;

	emit tile_unit_changed(old_tile_pos);
	emit tile_unit_changed(tile_pos);
}

bool battle::is_current_unit_in_enemy_range_at(const QPoint &tile_pos) const
{
	if (this->get_current_unit() == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const army *enemy_army = this->get_current_unit()->is_defender() ? this->attacking_army : this->defending_army;

	for (const military_unit *enemy : enemy_army->get_military_units()) {
		const battle_unit_info *enemy_info = this->get_unit_info(enemy);
		const QPoint enemy_tile_pos = enemy_info->get_tile_pos();
		const int distance = point::distance_to(enemy_tile_pos, tile_pos);

		if (distance <= enemy_info->get_range()) {
			return true;
		}
	}

	return false;
}

const site *battle::get_location() const
{
	assert_throw(ctx.dungeon_site != nullptr || this->scope != nullptr);

	if (ctx.dungeon_site != nullptr) {
		return ctx.dungeon_site;
	}

	return this->scope->get_game_data()->get_capital();
}

battle_tile::battle_tile(const terrain_type *base_terrain, const terrain_type *terrain)
	: combat_tile_base(base_terrain, terrain)
{
}

battle_unit_info::battle_unit_info(const military_unit *unit, const bool defender)
	: combat_unit_info_base(defender), unit(unit)
{
	connect(unit, &military_unit::icon_changed, this, &battle_unit_info::icon_changed);
	connect(unit, &military_unit::hit_points_changed, this, &battle_unit_info::hit_points_changed);
	connect(unit, &military_unit::max_hit_points_changed, this, &battle_unit_info::max_hit_points_changed);
}

const icon *battle_unit_info::get_icon() const
{
	return this->get_unit()->get_icon();
}

int battle_unit_info::get_hit_points() const
{
	return this->get_unit()->get_hit_points();
}

int battle_unit_info::get_max_hit_points() const
{
	return this->get_unit()->get_max_hit_points();
}

int battle_unit_info::get_range() const
{
	return this->get_unit()->get_stat(military_unit_stat::range).to_int();
}

const character *battle_unit_info::get_character() const
{
	return this->get_unit()->get_character();
}

bool battle_unit_info::is_player_unit() const
{
	return this->get_unit()->get_country() == game::get()->get_player_country();
}

bool battle_unit_info::is_player_enemy() const
{
	return this->get_unit()->get_country() != game::get()->get_player_country();
}

}
