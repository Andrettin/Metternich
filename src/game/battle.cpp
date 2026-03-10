#include "metternich.h"

#include "game/battle.h"

#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "script/effect/effect_list.h"
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
	connect(this, &battle::current_unit_changed, this, &battle::movable_tiles_changed);

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

	for (const military_unit *unit : attacking_army->get_military_units()) {
		max_range = std::max(max_range, unit->get_stat(military_unit_stat::range).to_int());
	}
	for (const military_unit *unit : defending_army->get_military_units()) {
		max_range = std::max(max_range, unit->get_stat(military_unit_stat::range).to_int());
	}

	return max_range;
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
		if (lhs->get_stat(military_unit_stat::movement) != rhs->get_stat(military_unit_stat::movement)) {
			return lhs->get_stat(military_unit_stat::movement) < rhs->get_stat(military_unit_stat::movement);
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
	while (!this->attacking_army->get_military_units().empty() && !this->defending_army->get_military_units().empty()) {
		co_await this->do_round();
	}

	this->result.attacker_victory = this->defending_army->get_military_units().empty();

	this->process_result();

	if (game::get()->get_current_combat() == this) {
		game::get()->set_current_combat(nullptr);
	}
}

QCoro::Task<void> battle::do_round()
{
	int attacker_initiative = 0;
	int defender_initiative = 0;
	while (attacker_initiative == defender_initiative) {
		attacker_initiative = random::get()->roll_dice(battle::initiative_dice);
		defender_initiative = random::get()->roll_dice(battle::initiative_dice);
	}

	if (attacker_initiative < defender_initiative) {
		co_await this->do_army_round(this->attacking_army, this->defending_army);
		co_await this->do_army_round(this->defending_army, this->attacking_army);
	} else {
		co_await this->do_army_round(this->defending_army, this->attacking_army);
		co_await this->do_army_round(this->attacking_army, this->defending_army);
	}
}

QCoro::Task<void> battle::do_army_round(metternich::army *army, metternich::army *enemy_army)
{
	if (army->get_military_units().empty()) {
		co_return;
	}

	if (enemy_army->get_military_units().empty() && army == this->defending_army) {
		co_return;
	}

	const std::vector<military_unit *> army_units = army->get_military_units();
	for (military_unit *unit : army_units) {
		if (enemy_army->get_military_units().empty() && army == this->defending_army) {
			break;
		}

		bool attacked = false;
		battle_unit_info *unit_info = this->get_unit_info(unit);
		unit_info->set_remaining_movement(unit->get_stat(military_unit_stat::movement).to_int());

		this->set_current_unit(unit_info);

		while (!attacked && unit_info->get_remaining_movement() > 0) {
			const QPoint current_tile_pos = unit_info->get_tile_pos();

			QPoint target_pos(-1, -1);

			if (army->get_domain() == game::get()->get_player_country() && !this->is_autoplay_enabled()) {
				emit movable_tiles_changed();

				this->target_promise = std::make_unique<QPromise<QPoint>>();
				const QFuture<QPoint> target_future = this->target_promise->future();
				this->target_promise->start();

				target_pos = co_await target_future;

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
				if (distance_to_target <= unit->get_stat(military_unit_stat::range).to_int()) {
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

			if (tile.unit != nullptr) {
				if (distance <= unit->get_stat(military_unit_stat::range).to_int() && vector::contains(enemy_army->get_military_units(), tile.unit)) {
					this->do_unit_attack(unit, tile.unit, enemy_army);
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

	if (this->get_current_unit() != nullptr) {
		this->set_current_unit(nullptr);
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

void battle::do_unit_attack(const military_unit *unit, military_unit *enemy, army *enemy_army)
{
	const int damage = 1;
	enemy->change_hit_points(-damage);

	const bool enemy_dead = !vector::contains(enemy_army->get_military_units(), enemy);
	if (enemy_dead) {
		this->remove_unit_info(enemy);
	}
}

void battle::process_result()
{
	const bool success = this->attacking_army->get_domain() == this->scope ? this->result.attacker_victory : !this->result.attacker_victory;

	context ctx = this->ctx;
	ctx.in_combat = false;

	if (this->scope == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->scope->get_government()->get_war_minister_portrait();

		if (success) {
			engine_interface::get()->add_combat_notification("Victory!", war_minister_portrait, std::format("You have won a battle!"));
		} else {
			engine_interface::get()->add_combat_notification("Defeat!", war_minister_portrait, std::format("You have lost a battle!"));
		}
	}
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

void battle::set_target(const QPoint &tile_pos)
{
	if (this->target_promise == nullptr) {
		return;
	}

	this->target_promise->addResult(tile_pos);
	this->target_promise->finish();
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

		if (distance <= enemy->get_stat(military_unit_stat::range).to_int()) {
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

}
