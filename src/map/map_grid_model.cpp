#include "metternich.h"

#include "map/map_grid_model.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "ui/icon.h"
#include "unit/civilian_unit.h"
#include "util/exception_util.h"
#include "util/point_util.h"

namespace metternich {

map_grid_model::map_grid_model()
{
	connect(map::get(), &map::tile_terrain_changed, this, &map_grid_model::on_tile_terrain_changed);
	connect(map::get(), &map::tile_improvement_changed, this, &map_grid_model::on_tile_improvement_changed);
	connect(map::get(), &map::tile_civilian_unit_changed, this, &map_grid_model::on_tile_civilian_unit_changed);
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
			case role::underlay_image_sources: {
				QStringList underlay_image_sources;

				if (tile->has_river() && tile->get_terrain()->is_water() && tile->get_river_frame() != -1) {
					QString river_image_source = "tile/";
					river_image_source += "river";
					river_image_source += "/" + QString::number(tile->get_river_frame());
					underlay_image_sources.push_back(std::move(river_image_source));
				}

				return underlay_image_sources;
			}
			case role::overlay_image_sources: {
				QStringList overlay_image_sources;

				if (tile->has_river() && !tile->get_terrain()->is_water() && tile->get_river_frame() != -1) {
					QString river_image_source = "tile/";
					river_image_source += "river";
					river_image_source += "/" + QString::number(tile->get_river_frame());
					overlay_image_sources.push_back(std::move(river_image_source));
				}

				if (tile->get_province() != nullptr && !tile->get_province()->is_water_zone()) {
					for (const direction direction : tile->get_border_directions()) {
						overlay_image_sources.push_back("tile/borders/province_border/" + QString::number(static_cast<int>(direction)));
					}
				}

				if (tile->get_settlement() != nullptr) {
					overlay_image_sources.push_back("tile/settlement/default");
				}
				
				if (tile->get_improvement() != nullptr) {
					QString image_source = "tile/improvement/" + tile->get_improvement()->get_identifier_qstring();

					if (tile->get_improvement()->has_terrain_image_filepath(tile->get_terrain())) {
						image_source += "/" + tile->get_terrain()->get_identifier_qstring();
					}

					image_source += "/" + QString::number(tile->get_improvement_variation());

					overlay_image_sources.push_back(std::move(image_source));
				} else if (tile->get_resource() != nullptr) {
					overlay_image_sources.push_back("icon/" + tile->get_resource()->get_icon()->get_identifier_qstring());
				}

				return overlay_image_sources;
			}
			case role::site:
				return QVariant::fromValue(const_cast<site *>(tile->get_site()));
			case role::province:
				return QVariant::fromValue(const_cast<province *>(tile->get_province()));
			case role::terrain:
				return QVariant::fromValue(const_cast<terrain_type *>(tile->get_terrain()));
			case role::resource:
				return QVariant::fromValue(const_cast<resource *>(tile->get_resource()));
			case role::improvement:
				return QVariant::fromValue(const_cast<improvement *>(tile->get_improvement()));
			case role::civilian_unit:
				return QVariant::fromValue(tile->get_civilian_unit());
			case role::upper_label: {
				const QPoint upper_tile_pos = tile_pos - QPoint(0, 1);

				if (!map::get()->contains(upper_tile_pos)) {
					return QString();
				}

				const metternich::tile *upper_tile = map::get()->get_tile(upper_tile_pos);

				if (upper_tile->get_settlement() != nullptr && upper_tile->get_province() != nullptr) {
					const province_game_data *upper_tile_province_game_data = upper_tile->get_province()->get_game_data();
					if (upper_tile_province_game_data->is_capital()) {
						return upper_tile->get_settlement()->get_game_data()->get_current_cultural_name_qstring();
					} else {
						return upper_tile_province_game_data->get_current_cultural_name_qstring();
					}
				}

				return QString();
			}
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
	emit dataChanged(index, index, {
		static_cast<int>(role::image_source),
		static_cast<int>(role::underlay_image_sources),
		static_cast<int>(role::overlay_image_sources),
		static_cast<int>(role::terrain)
	});
}

void map_grid_model::on_tile_improvement_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::overlay_image_sources),
		static_cast<int>(role::improvement)
	});
}

void map_grid_model::on_tile_civilian_unit_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::civilian_unit)
	});
}

}
