#include "metternich.h"

#include "map/map_grid_model.h"

#include "database/defines.h"
#include "economy/commodity.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/exception_util.h"
#include "util/point_util.h"

namespace metternich {

map_grid_model::map_grid_model()
{
	connect(map::get(), &map::tile_terrain_changed, this, &map_grid_model::on_tile_terrain_changed);
	connect(map::get(), &map::tile_culture_changed, this, &map_grid_model::on_tile_culture_changed);
}

QString map_grid_model::build_image_source(const terrain_type *terrain, const short tile_frame)
{
	QString image_source = "tile/terrain/" + terrain->get_identifier_qstring();

	image_source += "/" + QString::number(tile_frame);

	return image_source;
}

int map_grid_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return map::get()->get_height();
}

int map_grid_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return map::get()->get_width();
}

QVariant map_grid_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_grid_model::role model_role = static_cast<map_grid_model::role>(role);
		const QPoint tile_pos(index.column(), index.row());

		if (!map::get()->contains(tile_pos)) {
			throw std::runtime_error("Invalid tile position: " + point::to_string(tile_pos) + ".");
		}

		const tile *tile = map::get()->get_tile(tile_pos);

		switch (model_role) {
			case role::base_image_source:
				return map_grid_model::build_image_source(defines::get()->get_default_base_terrain(), tile->get_base_tile());
			case role::image_source:
				return map_grid_model::build_image_source(tile->get_terrain(), tile->get_tile());
			case role::overlay_image_sources: {
				QStringList overlay_image_sources;

				if (tile->get_settlement() != nullptr) {
					overlay_image_sources.push_back("tile/settlement/default");
				}
				
				if (tile->get_resource() != nullptr) {
					overlay_image_sources.push_back("icon/commodity/" + tile->get_resource()->get_identifier_qstring());
				}

				return overlay_image_sources;
			}
			case role::site_name:
				if (tile->get_site() != nullptr) {
					return tile->get_site()->get_game_data()->get_current_cultural_name_qstring();
				}

				return QString();
			case role::province_name:
				if (tile->get_province() != nullptr) {
					return tile->get_province()->get_game_data()->get_current_cultural_name_qstring();
				}

				return QString();
			default:
				throw std::runtime_error("Invalid map grid model role: " + std::to_string(role) + ".");
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
	}

	return QVariant();
}

void map_grid_model::on_tile_terrain_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, { static_cast<int>(role::image_source) });
}

void map_grid_model::on_tile_culture_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::site_name),
		static_cast<int>(role::province_name)
	});
}

}
