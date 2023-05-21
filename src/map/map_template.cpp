#include "metternich.h"

#include "map/map_template.h"

#include "database/defines.h"
#include "map/direction.h"
#include "map/map.h"
#include "map/map_projection.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/geoshape_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/vector_util.h"

namespace metternich {

//map templates must be initialized after sites, as sites add themselves to the world site list in their initialization function, and during map template initialization the sites are then added to the map template's site position map
//map templates must also be initialized after provinces, as they set their settlements to have them as their provinces, which is needed for site position initialization
const std::set<std::string> map_template::database_dependencies = { site::class_identifier, province::class_identifier };

void map_template::initialize()
{
	const QRect map_rect(QPoint(0, 0), this->get_size());

	if (!this->get_province_image_filepath().empty()) {
		const QImage province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		for (const site *site : this->get_world()->get_sites()) {
			assert_throw(site->get_geocoordinate().is_valid());

			if (site->get_geocoordinate().is_null()) {
				continue;
			}

			if (!this->get_georectangle().contains(site->get_geocoordinate())) {
				continue;
			}

			QPoint tile_pos = this->get_geocoordinate_pos(site->get_geocoordinate()) + site->get_pos_offset();

			if (!map_rect.contains(tile_pos)) {
				continue;
			}

			//if the site is not placed in its province, nudge its position to be in the nearest point in its province
			if (site->get_province() != nullptr && !province_image.isNull() && province::try_get_by_color(province_image.pixelColor(tile_pos)) != site->get_province()) {
				bool found_pos = false;
				int64_t best_distance = std::numeric_limits<int64_t>::max();

				QRect rect(tile_pos, QSize(1, 1));

				static constexpr int max_range = 2;
				for (int i = 0; i < max_range; ++i) {
					rect.setTopLeft(rect.topLeft() - QPoint(1, 1));
					rect.setBottomRight(rect.bottomRight() + QPoint(1, 1));

					bool checked_on_map = false;

					rect::for_each_edge_point(rect, [&](const QPoint &checked_pos) {
						if (!map_rect.contains(checked_pos)) {
							return;
						}

						checked_on_map = true;

						const province *checked_province = province::try_get_by_color(province_image.pixelColor(checked_pos));
						if (checked_province != site->get_province()) {
							return;
						}

						const geocoordinate checked_pos_geocoordinate = this->get_pos_geocoordinate(checked_pos);
						const int64_t distance = number::distance_between(checked_pos_geocoordinate.get_longitude().get_value(), checked_pos_geocoordinate.get_latitude().get_value(), site->get_geocoordinate().get_longitude().get_value(), site->get_geocoordinate().get_latitude().get_value());
						if (distance < best_distance) {
							best_distance = distance;
							tile_pos = checked_pos;
							found_pos = true;
						}
					});

					if (found_pos) {
						break;
					}

					if (!checked_on_map) {
						break;
					}
				}

				if (!found_pos) {
					log::log_error(std::format("No position found for site \"{}\" in province \"{}\".", site->get_identifier(), site->get_province()->get_identifier()));
					continue;
				}
			}

			assert_throw(map_rect.contains(tile_pos));

			if (this->sites_by_position.contains(tile_pos)) {
				throw std::runtime_error("Both the sites of \"" + this->sites_by_position.find(tile_pos)->second->get_identifier() + "\" and \"" + site->get_identifier() + "\" occupy the " + point::to_string(tile_pos) + " position in map template \"" + this->get_identifier() + "\".");
			}

			this->sites_by_position[tile_pos] = site;
		}
	} else {
		log::log_error(std::format("Map template \"{}\" has no province image filepath.", this->get_identifier()));
	}

	named_data_entry::initialize();
}

void map_template::check() const
{
	assert_throw(this->map_projection != nullptr);
	this->map_projection->validate_area(this->get_georectangle(), this->get_size());
}

QPoint map_template::get_geocoordinate_pos(const geocoordinate &geocoordinate) const
{
	return this->map_projection->geocoordinate_to_point(geocoordinate, this->get_georectangle(), this->get_size(), this->geocoordinate_x_offset);
}

geocoordinate map_template::get_pos_geocoordinate(const QPoint &pos) const
{
	return this->map_projection->point_to_geocoordinate(pos, this->get_georectangle(), this->get_size(), this->geocoordinate_x_offset);
}

void map_template::set_terrain_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_terrain_image_filepath()) {
		return;
	}

	this->terrain_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_terrain_image()
{
	assert_throw(this->get_world() != nullptr);

	terrain_geodata_map terrain_geodata_map = this->get_world()->parse_terrain_geojson_folder();

	color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

	for (auto &[terrain_variant, geoshapes] : terrain_geodata_map) {
		QColor color;

		if (std::holds_alternative<const terrain_feature *>(terrain_variant)) {
			const terrain_feature *terrain_feature = std::get<const metternich::terrain_feature *>(terrain_variant);

			if (terrain_feature->is_river()) {
				continue;
			}

			if (terrain_feature->is_hidden()) {
				continue;
			}

			color = terrain_feature->get_terrain_type()->get_color();
		} else {
			const terrain_type *terrain_type = std::get<const metternich::terrain_type *>(terrain_variant);
			color = terrain_type->get_color();
		}

		if (!color.isValid()) {
			throw std::runtime_error("Terrain variant has no valid color.");
		}

		vector::merge(geodata_map[color], std::move(geoshapes));
	}

	assert_throw(this->map_projection != nullptr);

	this->map_projection->validate_area(this->get_georectangle(), this->get_size());

	QImage base_image;

	if (!this->get_terrain_image_filepath().empty()) {
		base_image = QImage(path::to_qstring(this->get_terrain_image_filepath()));
		assert_throw(!base_image.isNull());
	} else {
		base_image = QImage(this->get_size(), QImage::Format_RGBA8888);
		base_image.fill(Qt::transparent);
	}

	QImage province_image;

	if (!this->get_province_image_filepath().empty()) {
		//write water zones
		province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		assert_throw(province_image.size() == base_image.size());

		for (int x = 0; x < province_image.width(); ++x) {
			for (int y = 0; y < province_image.height(); ++y) {
				const QColor province_color = province_image.pixelColor(x, y);

				if (province_color.alpha() == 0) {
					//ignore transparent pixels
					continue;
				}

				if (base_image.pixelColor(x, y).alpha() != 0) {
					//ignore already-written pixels
					continue;
				}

				const province *province = province::get_by_color(province_color);

				if (!province->is_water_zone()) {
					continue;
				}

				const terrain_type *terrain = defines::get()->get_default_water_zone_terrain();
				assert_throw(terrain != nullptr);

				const QColor terrain_color = terrain->get_color();

				assert_throw(terrain_color.isValid());

				base_image.setPixelColor(x, y, terrain_color);
			}
		}
	}

	//write terrain geoshapes
	std::filesystem::path output_filepath = this->get_terrain_image_filepath().filename();
	if (output_filepath.empty()) {
		output_filepath = "terrain.png";
	}

	geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);

	if (!this->get_province_image_filepath().empty()) {
		//write terrain based on provinces, for pixels without any terrain data set for them
		QImage output_image(path::to_qstring(output_filepath));

		assert_throw(!output_image.isNull());
		assert_throw(province_image.size() == output_image.size());

		for (int x = 0; x < province_image.width(); ++x) {
			for (int y = 0; y < province_image.height(); ++y) {
				const QColor province_color = province_image.pixelColor(x, y);

				if (province_color.alpha() == 0) {
					//ignore transparent pixels
					continue;
				}

				if (output_image.pixelColor(x, y).alpha() != 0) {
					//ignore already-written pixels
					continue;
				}

				const province *province = province::get_by_color(province_color);

				if (province->is_water_zone()) {
					continue;
				}

				const terrain_type *terrain = defines::get()->get_default_province_terrain();

				assert_throw(terrain != nullptr);

				const QColor terrain_color = terrain->get_color();

				assert_throw(terrain_color.isValid());

				output_image.setPixelColor(x, y, terrain_color);
			}
		}

		output_image.save(path::to_qstring(output_filepath));
	}
}

void map_template::set_river_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_river_image_filepath()) {
		return;
	}

	this->river_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_river_image()
{
	try {
		assert_throw(this->get_world() != nullptr);

		terrain_geodata_map terrain_geodata_map = this->get_world()->parse_terrain_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		for (auto &[terrain_variant, geoshapes] : terrain_geodata_map) {
			QColor color;

			if (std::holds_alternative<const terrain_feature *>(terrain_variant)) {
				const terrain_feature *terrain_feature = std::get<const metternich::terrain_feature *>(terrain_variant);

				if (!terrain_feature->is_river()) {
					continue;
				}

				if (terrain_feature->is_hidden()) {
					continue;
				}

				color = QColor(Qt::blue);
			} else {
				continue;
			}

			if (!color.isValid()) {
				throw std::runtime_error("River has no valid color.");
			}

			vector::merge(geodata_map[color], std::move(geoshapes));
		}

		assert_throw(this->map_projection != nullptr);

		this->map_projection->validate_area(this->get_georectangle(), this->get_size());

		QImage base_image;

		if (!this->get_river_image_filepath().empty()) {
			base_image = QImage(path::to_qstring(this->get_river_image_filepath()));
			assert_throw(!base_image.isNull());
		} else {
			base_image = QImage(this->get_size(), QImage::Format_RGBA8888);
			base_image.fill(Qt::transparent);
		}

		//write river geoshapes
		std::filesystem::path output_filepath = this->get_river_image_filepath().filename();
		if (output_filepath.empty()) {
			output_filepath = "rivers.png";
		}

		geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);
	} catch (const std::exception &exception) {
		exception::report(exception);
		std::terminate();
	}
}

void map_template::set_province_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_province_image_filepath()) {
		return;
	}

	this->province_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_province_image()
{
	using province_geodata_map_type = province_map<std::vector<std::unique_ptr<QGeoShape>>>;

	try {
		assert_throw(this->get_world() != nullptr);

		province_geodata_map_type province_geodata_map = this->get_world()->parse_provinces_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		for (auto &[province, geoshapes] : province_geodata_map) {
			const QColor color = province->get_color();
			if (!color.isValid()) {
				throw std::runtime_error("Province \"" + province->get_identifier() + "\" has no valid color.");
			}

			vector::merge(geodata_map[color], std::move(geoshapes));
		}

		assert_throw(this->map_projection != nullptr);

		this->map_projection->validate_area(this->get_georectangle(), this->get_size());

		QImage base_image;

		if (!this->get_province_image_filepath().empty()) {
			base_image = QImage(path::to_qstring(this->get_province_image_filepath()));
			assert_throw(!base_image.isNull());
		}

		std::filesystem::path output_filepath = this->get_province_image_filepath().filename();
		if (output_filepath.empty()) {
			output_filepath = "provinces.png";
		}

		geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);
	} catch (const std::exception &exception) {
		exception::report(exception);
		std::terminate();
	}
}

void map_template::apply() const
{
	map *map = map::get();
	map->clear();
	map->set_size(this->get_size());
	map->create_tiles();

	this->apply_terrain();
	this->apply_rivers();
	this->apply_provinces();
}

void map_template::apply_terrain() const
{
	assert_throw(!this->get_terrain_image_filepath().empty());

	const QImage terrain_image(path::to_qstring(this->get_terrain_image_filepath()));

	map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = terrain_image.pixelColor(tile_pos);

			if (tile_color.alpha() == 0) {
				continue;
			}

			const terrain_type *terrain = terrain_type::get_by_color(tile_color);
			map->set_tile_terrain(tile_pos, terrain);
		}
	}

	//apply site terrain
	for (const auto &[tile_pos, site] : this->sites_by_position) {
		if (site->get_terrain_type() == nullptr) {
			continue;
		}

		assert_throw(site->get_type() == site_type::terrain || site->get_type() == site_type::resource || site->get_type() == site_type::settlement);

		map->set_tile_terrain(tile_pos, site->get_terrain_type());
	}
}

void map_template::apply_rivers() const
{
	if (this->get_river_image_filepath().empty()) {
		return;
	}

	const QImage river_image(path::to_qstring(this->get_river_image_filepath()));

	map *map = map::get();

	static const QColor river_color = QColor(Qt::blue);

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = river_image.pixelColor(tile_pos);

			if (tile_color != river_color) {
				continue;
			}

			uint8_t base_direction_flags = direction_flag::none;

			point::for_each_adjacent(tile_pos, [map, &river_image, &tile_pos, &base_direction_flags](const QPoint &adjacent_pos) {
				if (!map->contains(adjacent_pos)) {
					return;
				}
				
				const QColor adjacent_tile_color = river_image.pixelColor(adjacent_pos);

				if (adjacent_tile_color != river_color) {
					return;
				}

				const direction direction = offset_to_direction(adjacent_pos - tile_pos);
				base_direction_flags |= direction_to_flag(direction);
			});

			if ((base_direction_flags & direction_flag::north) != 0) {
				if ((base_direction_flags & direction_flag::south) != 0) {
					if (
						(base_direction_flags & direction_flag::west) != 0
						&& (base_direction_flags & direction_flag::southeast) != 0
					) {
						map->add_tile_river_direction(tile_pos, direction::east);
						map->add_tile_river_direction(tile_pos, direction::south);
					} else if ((base_direction_flags & direction_flag::northwest) != 0) {
						map->add_tile_river_direction(tile_pos, direction::east);
						map->add_tile_river_direction(tile_pos, direction::northeast);
					} else {
						map->add_tile_river_direction(tile_pos, direction::east);
					}
				} else if ((base_direction_flags & direction_flag::west) != 0) {
					map->add_tile_river_direction(tile_pos, direction::east);
					map->add_tile_river_direction(tile_pos, direction::south);
					map->add_tile_river_direction(tile_pos, direction::southeast);
				} else if ((base_direction_flags & direction_flag::east) != 0) {
					if ((base_direction_flags & direction_flag::northwest) != 0) {
						map->add_tile_river_direction(tile_pos, direction::east);
						map->add_tile_river_direction(tile_pos, direction::north);
						map->add_tile_river_direction(tile_pos, direction::northeast);
					} else {
						map->add_tile_river_direction(tile_pos, direction::east);
					}
				} else {
					map->add_tile_river_direction(tile_pos, direction::east, true);
					map->add_tile_river_direction(tile_pos, direction::northeast, true);

					const QPoint east_tile_pos = tile_pos + QPoint(1, 0);
					if (map->contains(east_tile_pos)) {
						map->get_tile(east_tile_pos)->add_river_direction(direction::northwest);
					}
				}
			} else if ((base_direction_flags & direction_flag::south) != 0) {
				if ((base_direction_flags & direction_flag::west) != 0) {
					if ((base_direction_flags & direction_flag::northwest) != 0) {
						map->add_tile_river_direction(tile_pos, direction::west);
						map->add_tile_river_direction(tile_pos, direction::south);
						map->add_tile_river_direction(tile_pos, direction::southwest);
					} else {
						map->add_tile_river_direction(tile_pos, direction::south);
					}
				} else if ((base_direction_flags & direction_flag::east) != 0) {
					map->add_tile_river_direction(tile_pos, direction::southeast);
				} else {
					map->add_tile_river_direction(tile_pos, direction::east, true);
					map->add_tile_river_direction(tile_pos, direction::southeast, true);

					const QPoint east_tile_pos = tile_pos + QPoint(1, 0);
					if (map->contains(east_tile_pos)) {
						map->get_tile(east_tile_pos)->add_river_direction(direction::southwest);
					}
				}
			} else if ((base_direction_flags & direction_flag::west) != 0) {
				if ((base_direction_flags & direction_flag::east) != 0) {
					if ((base_direction_flags & direction_flag::northwest) != 0) {
						map->add_tile_river_direction(tile_pos, direction::south);
						map->add_tile_river_direction(tile_pos, direction::southwest);
					} else {
						map->add_tile_river_direction(tile_pos, direction::south);
					}
				} else {
					map->add_tile_river_direction(tile_pos, direction::south, true);
					map->add_tile_river_direction(tile_pos, direction::southwest, true);

					const QPoint south_tile_pos = tile_pos + QPoint(0, 1);
					if (map->contains(south_tile_pos)) {
						map->get_tile(south_tile_pos)->add_river_direction(direction::northwest);
					}
				}
			} else if ((base_direction_flags & direction_flag::east) != 0) {
				map->add_tile_river_direction(tile_pos, direction::south, true);
				map->add_tile_river_direction(tile_pos, direction::southeast, true);

				const QPoint south_tile_pos = tile_pos + QPoint(0, 1);
				if (map->contains(south_tile_pos)) {
					map->get_tile(south_tile_pos)->add_river_direction(direction::northeast);
				}
			}
		}
	}
}

void map_template::apply_provinces() const
{
	assert_throw(!this->get_province_image_filepath().empty());

	const QImage province_image(path::to_qstring(this->get_province_image_filepath()));

	map *map = map::get();

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = province_image.pixelColor(tile_pos);

			if (tile_color.alpha() == 0) {
				continue;
			}

			const province *province = province::get_by_color(tile_color);
			map->set_tile_province(tile_pos, province);
		}
	}

	//apply tile sites
	for (const auto &[tile_pos, site] : this->sites_by_position) {
		map->set_tile_site(tile_pos, site);
	}
}

}
