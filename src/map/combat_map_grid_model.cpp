#include "metternich.h"

#include "map/combat_map_grid_model.h"

#include "game/combat.h"
#include "game/game.h"
#include "map/terrain_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/point_util.h"

namespace metternich {

combat_map_grid_model::combat_map_grid_model()
{
}

QString combat_map_grid_model::build_image_source(const terrain_type *terrain, const short tile_frame)
{
	QString image_source = "tile/terrain/" + terrain->get_identifier_qstring();

	image_source += "/" + QString::number(tile_frame);

	return image_source;
}

int combat_map_grid_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return game::get()->get_current_combat()->get_map_height();
}

int combat_map_grid_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return game::get()->get_current_combat()->get_map_width();
}

QVariant combat_map_grid_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const combat_map_grid_model::role model_role = static_cast<combat_map_grid_model::role>(role);
		const QPoint tile_pos(index.column(), index.row());
		const combat *combat = game::get()->get_current_combat();

		if (!combat->get_map_rect().contains(tile_pos)) {
			throw std::runtime_error("Invalid tile position: " + point::to_string(tile_pos) + ".");
		}

		const combat_tile &tile = combat->get_tile(tile_pos);

		switch (model_role) {
			case role::base_image_sources: {
				QStringList image_sources;

				if (!combat->get_base_terrain()->get_subtiles().empty()) {
					for (const short subtile : tile.base_subtile_frames) {
						image_sources.push_back(combat_map_grid_model::build_image_source(combat->get_base_terrain(), subtile));
					}
				} else {
					image_sources.push_back(combat_map_grid_model::build_image_source(combat->get_base_terrain(), tile.base_tile_frame));
				}

				return image_sources;
			}
			case role::image_sources: {
				QStringList image_sources;

				if (!tile.terrain->get_subtiles().empty()) {
					for (const short subtile : tile.subtile_frames) {
						image_sources.push_back(combat_map_grid_model::build_image_source(tile.terrain, subtile));
					}
				} else {
					image_sources.push_back(combat_map_grid_model::build_image_source(tile.terrain, tile.tile_frame));
				}

				return image_sources;
			}
			case role::overlay_image_sources: {
				QStringList overlay_image_sources;
				return overlay_image_sources;
			}
			case role::terrain:
				return QVariant::fromValue(tile.terrain);
			default:
				throw std::runtime_error("Invalid combat map grid model role: " + std::to_string(role) + ".");
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

}
