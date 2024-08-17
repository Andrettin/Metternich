#include "metternich.h"

#include "map/map_grid_model.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "infrastructure/settlement_type.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "ui/icon.h"
#include "unit/civilian_unit.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/point_util.h"

namespace metternich {

map_grid_model::map_grid_model()
{
	connect(map::get(), &map::tile_terrain_changed, this, &map_grid_model::on_tile_terrain_changed);
	connect(map::get(), &map::tile_exploration_changed, this, &map_grid_model::on_tile_exploration_changed);
	connect(map::get(), &map::tile_prospection_changed, this, &map_grid_model::on_tile_prospection_changed);
	connect(map::get(), &map::tile_resource_changed, this, &map_grid_model::on_tile_resource_changed);
	connect(map::get(), &map::tile_settlement_type_changed, this, &map_grid_model::on_tile_settlement_type_changed);
	connect(map::get(), &map::tile_improvement_changed, this, &map_grid_model::on_tile_improvement_changed);
	connect(map::get(), &map::tile_pathway_changed, this, &map_grid_model::on_tile_pathway_changed);
	connect(map::get(), &map::tile_transport_level_changed, this, &map_grid_model::on_tile_transport_level_changed);
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
			case role::base_image_sources: {
				QStringList image_sources;

				if (!defines::get()->get_default_base_terrain()->get_subtiles().empty()) {
					for (const short subtile : tile->get_base_subtiles()) {
						image_sources.push_back(map_grid_model::build_image_source(defines::get()->get_default_base_terrain(), subtile));
					}
				} else {
					image_sources.push_back(map_grid_model::build_image_source(defines::get()->get_default_base_terrain(), tile->get_base_tile()));
				}

				return image_sources;
			}
			case role::image_sources: {
				QStringList image_sources;

				if (!tile->get_terrain()->get_subtiles().empty()) {
					for (const short subtile : tile->get_subtiles()) {
						image_sources.push_back(map_grid_model::build_image_source(tile->get_terrain(), subtile));
					}
				} else {
					image_sources.push_back(map_grid_model::build_image_source(tile->get_terrain(), tile->get_tile()));
				}

				return image_sources;
			}
			case role::underlay_image_sources: {
				QStringList underlay_image_sources;

				if (tile->has_river() && tile->get_terrain()->is_water()) {
					for (const short river_subtile_frame : tile->get_river_subtile_frames()) {
						QString river_image_source = "tile/";
						river_image_source += "river";
						river_image_source += "/" + QString::number(river_subtile_frame);
						underlay_image_sources.push_back(std::move(river_image_source));
					}
				}

				return underlay_image_sources;
			}
			case role::overlay_image_sources: {
				QStringList overlay_image_sources;

				if (tile->has_river() && !tile->get_terrain()->is_water()) {
					for (const short river_subtile_frame : tile->get_river_subtile_frames()) {
						QString river_image_source = "tile/";
						river_image_source += "river";
						river_image_source += "/" + QString::number(river_subtile_frame);
						overlay_image_sources.push_back(std::move(river_image_source));
					}
				}

				return overlay_image_sources;
			}
			case role::object_image_sources: {
				QStringList object_image_sources;

				if (tile->has_route()) {
					for (const auto &[pathway, frame] : tile->get_pathway_frames()) {
						QString pathway_image_source = "tile/pathway/" + pathway->get_identifier_qstring();
						pathway_image_source += "/" + QString::number(frame);
						object_image_sources.push_back(std::move(pathway_image_source));
					}
				}

				if (tile->get_province() != nullptr && !tile->get_province()->is_water_zone()) {
					for (const direction direction : tile->get_border_directions()) {
						object_image_sources.push_back("tile/borders/province_border/" + QString::number(static_cast<int>(direction)));
					}
				}

				if (tile->get_settlement_type() != nullptr) {
					QString image_source = "tile/settlement/" + tile->get_settlement_type()->get_identifier_qstring() + "/0";
					object_image_sources.push_back(std::move(image_source));
				} else if (tile->get_site() != nullptr && tile->get_site()->get_game_data()->get_main_improvement() != nullptr) {
					QString image_source = "tile/improvement/" + tile->get_site()->get_game_data()->get_main_improvement()->get_identifier_qstring();

					if (tile->get_site()->get_game_data()->get_main_improvement()->has_terrain_image_filepath(tile->get_terrain())) {
						image_source += "/" + tile->get_terrain()->get_identifier_qstring();
					}

					image_source += "/" + QString::number(tile->get_improvement_variation());

					object_image_sources.push_back(std::move(image_source));
				} else if (tile->get_resource() != nullptr && tile->is_resource_discovered()) {
					object_image_sources.push_back("icon/" + tile->get_resource()->get_icon()->get_identifier_qstring());
				}

				if (!game::get()->get_player_country()->get_game_data()->is_tile_explored(tile_pos)) {
					object_image_sources.push_back(map_grid_model::build_image_source(defines::get()->get_unexplored_terrain(), 0));
				}

				return object_image_sources;
			}
			case role::site:
				if (!game::get()->get_player_country()->get_game_data()->is_tile_explored(tile_pos)) {
					return QVariant::fromValue(nullptr);
				}

				return QVariant::fromValue(tile->get_site());
			case role::province:
				return QVariant::fromValue(tile->get_province());
			case role::terrain:
				return QVariant::fromValue(tile->get_terrain());
			case role::river:
				return tile->has_river();
			case role::resource:
				if (!tile->is_resource_discovered()) {
					return QVariant::fromValue(nullptr);
				}

				return QVariant::fromValue(tile->get_resource());
			case role::improvement:
				if (tile->get_site() == nullptr) {
					return QVariant::fromValue(nullptr);
				}

				return QVariant::fromValue(tile->get_site()->get_game_data()->get_main_improvement());
			case role::pathway:
				return QVariant::fromValue(tile->get_best_pathway());
			case role::transport_level:
				return QVariant::fromValue(tile->get_transport_level());
			case role::sea_transport_level:
				return QVariant::fromValue(tile->get_sea_transport_level());
			case role::civilian_unit:
				if (!game::get()->get_player_country()->get_game_data()->is_tile_explored(tile_pos)) {
					return QVariant::fromValue(nullptr);
				}

				return QVariant::fromValue(tile->get_civilian_unit());
			case role::upper_label: {
				const QPoint upper_tile_pos = tile_pos - QPoint(0, 1);

				if (!map::get()->contains(upper_tile_pos)) {
					return QString();
				}

				if (!game::get()->get_player_country()->get_game_data()->is_tile_explored(upper_tile_pos)) {
					return QString();
				}

				const metternich::tile *upper_tile = map::get()->get_tile(upper_tile_pos);
				const province *upper_tile_province = upper_tile->get_province();

				if (upper_tile_province != nullptr && upper_tile_province->get_game_data()->get_center_tile_pos() == upper_tile_pos) {
					const province_game_data *upper_tile_province_game_data = upper_tile_province->get_game_data();
					if (upper_tile_province_game_data->is_capital() && (upper_tile_province_game_data->get_owner()->get_game_data()->get_province_count() > 1 || upper_tile_province_game_data->get_owner()->is_great_power())) {
						assert_throw(upper_tile->get_settlement() != nullptr);
						return upper_tile->get_settlement()->get_game_data()->get_current_cultural_name_qstring();
					} else {
						return upper_tile_province_game_data->get_current_cultural_name_qstring();
					}
				}

				return QString();
			}
			case role::prospected:
				return game::get()->get_player_country()->get_game_data()->is_tile_prospected(tile_pos);
			default:
				throw std::runtime_error("Invalid map grid model role: " + std::to_string(role) + ".");
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

void map_grid_model::on_tile_terrain_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::image_sources),
		static_cast<int>(role::underlay_image_sources),
		static_cast<int>(role::overlay_image_sources),
		static_cast<int>(role::terrain)
	});
}

void map_grid_model::on_tile_exploration_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::object_image_sources),
		static_cast<int>(role::site),
		static_cast<int>(role::civilian_unit),
		static_cast<int>(role::upper_label)
	});
}

void map_grid_model::on_tile_prospection_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::prospected)
	});
}

void map_grid_model::on_tile_resource_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::object_image_sources),
		static_cast<int>(role::resource)
	});
}

void map_grid_model::on_tile_settlement_type_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::object_image_sources)
	});
}

void map_grid_model::on_tile_improvement_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::object_image_sources),
		static_cast<int>(role::improvement)
	});
}

void map_grid_model::on_tile_pathway_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::object_image_sources),
		static_cast<int>(role::pathway)
	});
}

void map_grid_model::on_tile_transport_level_changed(const QPoint &tile_pos)
{
	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, {
		static_cast<int>(role::transport_level),
		static_cast<int>(role::sea_transport_level)
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
