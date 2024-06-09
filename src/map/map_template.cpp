#include "metternich.h"

#include "map/map_template.h"

#include "database/defines.h"
#include "economy/resource.h"
#include "map/direction.h"
#include "map/map.h"
#include "map/map_projection.h"
#include "map/province.h"
#include "map/route.h"
#include "map/route_container.h"
#include "map/route_game_data.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/site_type.h"
#include "map/terrain_adjacency.h"
#include "map/terrain_adjacency_type.h"
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

const std::set<std::string> map_template::database_dependencies = {
	//map templates must be initialized after sites, as sites add themselves to the world site list in their initialization function, and during map template initialization the sites are then added to the map template's site position map
	site::class_identifier,

	//map templates must also be initialized after provinces, as they set their settlements to have them as their provinces, which is needed for site position initialization
	province::class_identifier
};

bool map_template::is_site_in_province(const site *site, const province *province, const province_geodata_map_type &province_geodata_map)
{
	const auto find_iterator = province_geodata_map.find(province);

	if (find_iterator == province_geodata_map.end()) {
		return false;
	}

	const std::vector<std::unique_ptr<QGeoShape>> &province_geoshapes = find_iterator->second;

	const QGeoCoordinate qgeocoordinate = site->get_geocoordinate().to_qgeocoordinate();

	for (const std::unique_ptr<QGeoShape> &geoshape : province_geoshapes) {
		if (geoshape->contains(qgeocoordinate)) {
			return true;
		}
	}

	return false;
}

void map_template::initialize()
{
	const QRect map_rect(QPoint(0, 0), this->get_size());

	if (!this->get_province_image_filepath().empty()) {
		const QImage province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		const province_geodata_map_type province_geodata_map = this->get_world()->parse_provinces_geojson_folder();

		for (const site *site : this->get_world()->get_sites()) {
			if (site->get_type() == site_type::none) {
				continue;
			}

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

			const province *tile_province = province::try_get_by_color(province_image.pixelColor(tile_pos));
			const province *site_province = tile_province;

			if (site->get_province() != nullptr) {
				site_province = site->get_province();
			} else {
				province_set adjacent_provinces;
				point::for_each_adjacent(tile_pos, [&site_province, &map_rect, &province_image, site, &province_geodata_map, &adjacent_provinces, tile_province](const QPoint &adjacent_pos) {
					if (!map_rect.contains(adjacent_pos)) {
						return;
					}

					const province *adjacent_province = province::try_get_by_color(province_image.pixelColor(adjacent_pos));

					if (adjacent_province == nullptr) {
						return;
					}

					if (adjacent_province == tile_province) {
						return;
					}

					if (adjacent_province->is_water_zone()) {
						return;
					}

					if (adjacent_provinces.contains(adjacent_province)) {
						return;
					}

					adjacent_provinces.insert(adjacent_province);
				});

				if (!adjacent_provinces.empty() && (tile_province == nullptr || !map_template::is_site_in_province(site, tile_province, province_geodata_map))) {
					for (const province *adjacent_province : adjacent_provinces) {
						if (map_template::is_site_in_province(site, adjacent_province, province_geodata_map)) {
							site_province = adjacent_province;
							break;
						}
					}
				}
			}

			//if the site is not placed in its province, nudge its position to be in the nearest point in its province
			if (site_province != tile_province && !province_image.isNull()) {
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
						if (checked_province != site_province) {
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
					log::log_error(std::format("No position found for site \"{}\" in province \"{}\".", site->get_identifier(), site_province->get_identifier()));
					continue;
				}
			}

			assert_throw(map_rect.contains(tile_pos));

			if (this->sites_by_position.contains(tile_pos)) {
				log::log_error(std::format("Both the sites of \"{}\" and \"{}\" occupy the {} position in map template \"{}\".", this->sites_by_position.find(tile_pos)->second->get_identifier(), site->get_identifier(), point::to_string(tile_pos), this->get_identifier()));
				continue;
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

			if (terrain_feature->is_river() || terrain_feature->is_border_river()) {
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
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

void map_template::set_border_river_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_border_river_image_filepath()) {
		return;
	}

	this->border_river_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_border_river_image()
{
	try {
		assert_throw(this->get_world() != nullptr);

		terrain_geodata_map terrain_geodata_map = this->get_world()->parse_terrain_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		static const QColor river_color = QColor(Qt::blue);

		for (auto &[terrain_variant, geoshapes] : terrain_geodata_map) {
			QColor color;

			if (std::holds_alternative<const terrain_feature *>(terrain_variant)) {
				const terrain_feature *terrain_feature = std::get<const metternich::terrain_feature *>(terrain_variant);

				if (!terrain_feature->is_border_river()) {
					continue;
				}

				if (terrain_feature->is_hidden()) {
					continue;
				}

				color = river_color;
			} else {
				continue;
			}

			if (!color.isValid()) {
				throw std::runtime_error("Border river has no valid color.");
			}

			vector::merge(geodata_map[color], std::move(geoshapes));
		}

		assert_throw(this->map_projection != nullptr);

		this->map_projection->validate_area(this->get_georectangle(), this->get_size());

		QImage base_image(this->get_size(), QImage::Format_RGBA8888);
		base_image.fill(Qt::transparent);

		//write border river geoshapes
		geoshape::write_to_image(base_image, geodata_map, this->get_georectangle(), this->map_projection, this->geocoordinate_x_offset);

		QImage image;
		if (!this->get_border_river_image_filepath().empty()) {
			image = QImage(path::to_qstring(this->get_border_river_image_filepath()));
			assert_throw(!image.isNull());
		} else {
			image = QImage(this->get_size() / 2, QImage::Format_RGBA8888);
			image.fill(Qt::transparent);
		}

		for (int x = 0; x < base_image.width(); ++x) {
			for (int y = 0; y < base_image.height(); ++y) {
				const QPoint tile_pos(x, y);
				const QColor tile_color = base_image.pixelColor(tile_pos);

				if (tile_color != river_color) {
					continue;
				}

				image.setPixelColor(x / 2, y / 2, tile_color);
			}
		}

		std::filesystem::path output_filepath = this->get_border_river_image_filepath().filename();
		if (output_filepath.empty()) {
			output_filepath = "border_rivers.png";
		}

		image.save(path::to_qstring(output_filepath));
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

void map_template::set_route_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_route_image_filepath()) {
		return;
	}

	this->route_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_route_image()
{
	using route_geodata_map_type = route_map<std::vector<std::unique_ptr<QGeoShape>>>;

	try {
		assert_throw(this->get_world() != nullptr);

		route_geodata_map_type route_geodata_map = this->get_world()->parse_routes_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		for (auto &[route, geoshapes] : route_geodata_map) {
			if (route->is_hidden()) {
				continue;
			}

			vector::merge(geodata_map[route->get_color()], std::move(geoshapes));
		}

		assert_throw(this->map_projection != nullptr);

		this->map_projection->validate_area(this->get_georectangle(), this->get_size());

		QImage base_image;

		if (!this->get_route_image_filepath().empty()) {
			base_image = QImage(path::to_qstring(this->get_route_image_filepath()));
			assert_throw(!base_image.isNull());
		} else {
			base_image = QImage(this->get_size(), QImage::Format_RGBA8888);
			base_image.fill(Qt::transparent);
		}

		//write route geoshapes
		std::filesystem::path output_filepath = this->get_route_image_filepath().filename();
		if (output_filepath.empty()) {
			output_filepath = "routes.png";
		}

		geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image, this->geocoordinate_x_offset);
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
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
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
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
	this->apply_border_rivers();
	this->apply_routes();
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
		const terrain_type *site_terrain = site->get_terrain_type();
		const resource *site_resource = site->get_map_data()->get_resource();

		//use a fallback terrain if the tile's terrain doesn't match the site's resource
		if (site_terrain == nullptr && site_resource != nullptr) {
			const tile *tile = map->get_tile(tile_pos);
			const terrain_type *tile_terrain = tile->get_terrain();
			const std::vector<const terrain_type *> &resource_terrains = site_resource->get_terrain_types();

			if (tile_terrain != nullptr && !vector::contains(resource_terrains, tile_terrain)) {
				site_terrain = site_resource->get_fallback_terrain(tile_terrain);
			}
		}

		if (site_terrain == nullptr) {
			continue;
		}

		assert_throw(site->get_type() == site_type::terrain || site->get_type() == site_type::resource || site->get_type() == site_type::settlement);

		map->set_tile_terrain(tile_pos, site_terrain);
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

			tile *tile = map::get()->get_tile(tile_pos);
			tile->set_inner_river(true);
		}
	}
}

void map_template::apply_border_rivers() const
{
	if (this->get_border_river_image_filepath().empty()) {
		return;
	}

	const QImage border_river_image(path::to_qstring(this->get_border_river_image_filepath()));
	const QRect border_river_image_rect = border_river_image.rect();

	map *map = map::get();

	static const QColor river_color = QColor(Qt::blue);

	for (int x = 0; x < border_river_image.width(); ++x) {
		for (int y = 0; y < border_river_image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = border_river_image.pixelColor(pixel_pos);

			if (pixel_color != river_color) {
				continue;
			}

			const QPoint river_tile_pos_1(x * 2, y * 2);
			const QPoint river_tile_pos_2 = river_tile_pos_1 + QPoint(1, 0);
			const QPoint river_tile_pos_3 = river_tile_pos_1 + QPoint(0, 1);
			const QPoint river_tile_pos_4 = river_tile_pos_1 + QPoint(1, 1);

			tile *river_tile_1 = map->get_tile(river_tile_pos_1);
			tile *river_tile_2 = map->get_tile(river_tile_pos_2);
			tile *river_tile_3 = map->get_tile(river_tile_pos_3);
			tile *river_tile_4 = map->get_tile(river_tile_pos_4);

			terrain_adjacency river_adjacency;

			river_tile_1->add_river_direction(direction::southeast);
			river_tile_2->add_river_direction(direction::southwest);
			river_tile_3->add_river_direction(direction::northeast);
			river_tile_4->add_river_direction(direction::northwest);

			static constexpr size_t direction_count = static_cast<size_t>(direction::count);

			for (size_t i = 0; i < direction_count; ++i) {
				const direction direction = static_cast<archimedes::direction>(i);
				const QPoint offset = direction_to_offset(direction);

				const QPoint adjacent_pixel_pos = pixel_pos + offset;
				terrain_adjacency_type adjacency_type = terrain_adjacency_type::other;

				if (border_river_image_rect.contains(adjacent_pixel_pos)) {
					if (border_river_image.pixelColor(adjacent_pixel_pos) == river_color) {
						adjacency_type = terrain_adjacency_type::same;
					} else {
						adjacency_type = terrain_adjacency_type::other;
					}
				} else {
					adjacency_type = terrain_adjacency_type::same;
				}

				river_adjacency.set_direction_adjacency_type(direction, adjacency_type);
			}

			if (river_adjacency.get_direction_adjacency_type(direction::north) == terrain_adjacency_type::same) {
				river_tile_1->add_river_direction(direction::east);
				river_tile_1->add_river_direction(direction::northeast);
				river_tile_2->add_river_direction(direction::west);
				river_tile_2->add_river_direction(direction::northwest);
			}

			if (river_adjacency.get_direction_adjacency_type(direction::south) == terrain_adjacency_type::same) {
				river_tile_3->add_river_direction(direction::east);
				river_tile_3->add_river_direction(direction::southeast);
				river_tile_4->add_river_direction(direction::west);
				river_tile_4->add_river_direction(direction::southwest);
			}

			if (river_adjacency.get_direction_adjacency_type(direction::west) == terrain_adjacency_type::same) {
				river_tile_1->add_river_direction(direction::south);
				river_tile_1->add_river_direction(direction::southwest);
				river_tile_3->add_river_direction(direction::north);
				river_tile_3->add_river_direction(direction::northwest);
			}

			if (river_adjacency.get_direction_adjacency_type(direction::east) == terrain_adjacency_type::same) {
				river_tile_2->add_river_direction(direction::south);
				river_tile_2->add_river_direction(direction::southeast);
				river_tile_4->add_river_direction(direction::north);
				river_tile_4->add_river_direction(direction::northeast);
			}
		}
	}
}

void map_template::apply_routes() const
{
	if (this->get_route_image_filepath().empty()) {
		return;
	}

	const QImage route_image(path::to_qstring(this->get_route_image_filepath()));

	map *map = map::get();

	static const QColor empty_color(Qt::black);

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = route_image.pixelColor(tile_pos);

			if (tile_color.alpha() == 0 || tile_color == empty_color) {
				continue;
			}

			if (map->is_tile_water(tile_pos)) {
				continue;
			}

			const route *route = route::get_by_color(tile_color);
			route->get_game_data()->add_tile(tile_pos);

			point::for_each_cardinally_adjacent(tile_pos, [map, &route_image, &tile_pos](const QPoint &adjacent_pos) {
				if (!map->contains(adjacent_pos)) {
					return;
				}
				
				const QColor adjacent_tile_color = route_image.pixelColor(adjacent_pos);

				if (adjacent_tile_color.alpha() == 0 || adjacent_tile_color == empty_color) {
					return;
				}

				if (map->is_tile_water(adjacent_pos)) {
					return;
				}

				const direction direction = offset_to_direction(adjacent_pos - tile_pos);
				map->add_tile_route_direction(tile_pos, direction);
			});
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
