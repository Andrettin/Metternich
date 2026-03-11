#include "metternich.h"

#include "game/combat_base.h"

#include "database/defines.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "util/vector_random_util.h"

namespace metternich {

combat_base::combat_base()
{
	this->base_terrain = defines::get()->get_default_base_terrain();

	this->promise = std::make_unique<QPromise<bool>>();
}

void combat_base::set_map_size(const QSize &map_size)
{
	QSize min_map_size = (QGuiApplication::primaryScreen()->size() / defines::get()->get_scaled_tile_size()) - QSize(1, 0);

	const int max_range = this->get_max_range_of_units();

	min_map_size.setWidth(std::max(min_map_size.width(), max_range + 4));

	this->map_rect = QRect(QPoint(0, 0), QSize(std::max(map_size.width(), min_map_size.width()), std::max(map_size.height(), min_map_size.height())));
}

void combat_base::set_base_terrain(const terrain_type *terrain)
{
	assert_throw(terrain != nullptr);

	this->base_terrain = terrain;
}

bool combat_base::is_tile_attacker_escape(const QPoint &tile_pos) const
{
	//can only retreat if the enemy is still present; this is to prevent a retreat from happening while opening chests, leading to potentially opening the same chest multiple times
	return tile_pos.x() == 0 && !this->is_defender_defeated();
}

bool combat_base::is_tile_defender_escape(const QPoint &tile_pos) const
{
	return tile_pos.x() == (this->get_map_width() - 1) && !this->is_attacker_defeated();
}

std::string combat_base::get_tile_text(const QPoint &tile_pos) const
{
	return std::format("({}, {})", tile_pos.x(), tile_pos.y());
}

QCoro::Task<QPoint> combat_base::get_target()
{
	this->target_promise = std::make_unique<QPromise<QPoint>>();
	const QFuture<QPoint> target_future = this->target_promise->future();
	this->target_promise->start();

	co_return co_await target_future;
}

void combat_base::set_target(const QPoint &tile_pos)
{
	if (this->target_promise == nullptr) {
		return;
	}

	this->target_promise->addResult(tile_pos);
	this->target_promise->finish();
}

bool combat_base::can_current_unit_move_to(const QPoint &tile_pos) const
{
	if (this->get_current_unit() == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	const QPoint current_tile_pos = this->get_current_unit()->get_tile_pos();

	const int distance = point::distance_to(current_tile_pos, tile_pos);

	if (distance > this->get_current_unit()->get_remaining_movement()) {
		return false;
	}

	const combat_tile_base &tile = this->get_tile(tile_pos);
	if (tile.is_occupied()) {
		return false;
	}

	return true;
}

bool combat_base::can_current_unit_retreat_at(const QPoint &tile_pos) const
{
	if (this->get_current_unit() == nullptr) {
		return false;
	}

	if (!this->get_map_rect().contains(tile_pos)) {
		return false;
	}

	if (this->get_current_unit()->is_defender()) {
		return this->defender_retreat_allowed && this->is_tile_defender_escape(tile_pos);
	} else {
		return this->attacker_retreat_allowed && this->is_tile_attacker_escape(tile_pos);
	}
}

combat_tile_base::combat_tile_base(const terrain_type *base_terrain, const terrain_type *terrain)
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
