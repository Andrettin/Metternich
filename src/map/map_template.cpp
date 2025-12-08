#include "metternich.h"

#include "map/map_template.h"

#include "database/database.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "economy/resource_container.h"
#include "map/direction.h"
#include "map/map.h"
#include "map/map_generator.h"
#include "map/map_projection.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/region.h"
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
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/geoshape_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/set_util.h"
#include "util/size_util.h"
#include "util/vector_random_util.h"
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
	if (!this->get_province_image_filepath().empty()) {
		const QRect map_rect(QPoint(0, 0), this->get_size());

		const QImage province_image = QImage(path::to_qstring(this->get_province_image_filepath()));

		for (const site *site : this->get_world()->get_sites()) {
			QPoint tile_pos = site->get_pos();

			if (tile_pos == point::invalid_point) {
				assert_throw(site->get_geocoordinate().is_valid());

				if (!site->get_geocoordinate().is_null() && this->get_georectangle().contains(site->get_geocoordinate())) {
					tile_pos = this->get_geocoordinate_pos(site->get_geocoordinate()) + site->get_pos_offset();
				}
			}

			if (tile_pos == point::invalid_point) {
				continue;
			}

			if (tile_pos.y() < 0) {
				tile_pos.setY(this->get_size().height() - 1 + tile_pos.y());
			}

			if (!map_rect.contains(tile_pos)) {
				continue;
			}

			const province *tile_province = province::try_get_by_color(province_image.pixelColor(tile_pos));
			const province *site_province = tile_province;

			if (site->get_province() != nullptr) {
				site_province = site->get_province();
			} else {
				continue;
			}

			if (site->get_type() == site_type::none || site->get_type() == site_type::resource) {
				continue;
			}

			//if the site is not placed in its province, nudge its position to be in the nearest point in its province; also nudge sites if they are too close to other sites
			const bool is_pos_valid = (site_province == tile_province || province_image.isNull()) && this->is_pos_available_for_site(tile_pos, site_province, province_image);
			if (!is_pos_valid) {
				bool found_pos = false;
				int64_t best_distance = std::numeric_limits<int64_t>::max();

				static constexpr int max_range = 16;
				for (int i = 1; i <= max_range; ++i) {
					const QRect rect(tile_pos - QPoint(i, i), tile_pos + QPoint(i, i));

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

						if (!this->is_pos_available_for_site(checked_pos, site_province, province_image)) {
							return;
						}

						const int64_t distance = point::square_distance_to(tile_pos, checked_pos);
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
	}

	named_data_entry::initialize();
}

void map_template::check() const
{
	if (this->map_projection != nullptr) {
		this->map_projection->validate_area(this->get_georectangle(), this->get_size());
	}

	if (this->get_world() == nullptr && !this->is_universe()) {
		throw std::runtime_error(std::format("Map template \"{}\" has neither a world, nor is it a universe map template.", this->get_identifier()));
	}
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
			assert_throw(color.isValid());
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

	if (this->is_randomly_generated()) {
		map_generator map_generator(this);
		map_generator.generate();
	} else {
		this->apply_terrain();
		this->apply_site_terrain();
		this->apply_rivers();
		this->apply_border_rivers();
		this->apply_routes();
		this->apply_provinces();
	}

	this->generate_additional_sites();
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
}

void map_template::apply_site_terrain() const
{
	map *map = map::get();

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

		assert_throw(site->get_type() == site_type::resource || site->get_type() == site_type::holding);

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

	std::map<const province *, int> province_tile_counts;

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = province_image.pixelColor(tile_pos);

			if (tile_color.alpha() == 0) {
				continue;
			}

			const province *province = province::get_by_color(tile_color);
			++province_tile_counts[province];
		}
	}

	std::map<const province *, int> province_holding_counts;
	for (const auto &[tile_pos, site] : this->sites_by_position) {
		assert_throw(site->get_province() != nullptr);
		if (site->is_settlement()) {
			++province_holding_counts[site->get_province()];
		}
	}

	static constexpr int min_province_tile_count = 96;

	for (int x = 0; x < map->get_width(); ++x) {
		for (int y = 0; y < map->get_height(); ++y) {
			const QPoint tile_pos(x, y);
			const QColor tile_color = province_image.pixelColor(tile_pos);

			if (tile_color.alpha() == 0) {
				continue;
			}

			const province *province = province::get_by_color(tile_color);

			if (province_tile_counts.find(province)->second < min_province_tile_count) {
				//do not place provinces if they would be too small
				continue;
			}

			if (!province->is_water_zone() && !province_holding_counts.contains(province)) {
				//do not place land provinces if they would have no holdings
				continue;
			}

			map->set_tile_province(tile_pos, province);
		}
	}

	//apply tile sites
	for (const auto &[tile_pos, site] : this->sites_by_position) {
		map->set_tile_site(tile_pos, site);
	}
}

void map_template::generate_additional_sites() const
{
	if (this->get_world() == nullptr) {
		return;
	}

	//generate sites which belong to other worlds, but can be generated on this one
	std::vector<const site *> potential_sites;
	for (const site *site : site::get_all()) {
		if (site->get_map_data()->is_on_map()) {
			continue;
		}

		if (site->get_world() == this->get_world()) {
			continue;
		}

		if (!site->can_be_generated_on_world(this->get_world())) {
			continue;
		}

		potential_sites.push_back(site);
	}

	vector::shuffle(potential_sites);

	for (const site *site : potential_sites) {
		this->generate_site(site);
	}
}

void map_template::generate_site(const site *site) const
{
	std::set<const province *> province_set;

	for (const region *region : site->get_generation_regions()) {
		set::merge(province_set, region->get_provinces());
	}

	std::vector<const province *> potential_provinces = container::to_vector(province_set);
	vector::shuffle(potential_provinces);

	map *map = map::get();

	for (const province *province : potential_provinces) {
		const province_map_data *province_map_data = province->get_map_data();
		if (!province_map_data->is_on_map()) {
			continue;
		}

		const resource *resource = site->get_map_data()->get_resource();
		const bool is_near_water = resource != nullptr && resource->is_near_water();
		const bool is_coastal = resource != nullptr && resource->is_coastal();
		const std::vector<const terrain_type *> &site_terrains = site->get_terrain_types();

		const std::vector<QPoint> province_tiles = vector::shuffled(province_map_data->get_tiles());

		if (!site_terrains.empty()) {
			bool has_terrain = false;
			for (const terrain_type *terrain : site_terrains) {
				if (province_map_data->get_tile_terrain_counts().contains(terrain)) {
					has_terrain = true;
					break;
				}
			}
			if (!has_terrain) {
				continue;
			}
		}

		for (const QPoint &tile_pos : province_tiles) {
			const tile *tile = map->get_tile(tile_pos);
			if (tile->get_site() != nullptr) {
				continue;
			}

			if (!site_terrains.empty() && !vector::contains(site_terrains, tile->get_terrain())) {
				continue;
			}

			if (is_coastal) {
				if (!map->is_tile_coastal(tile_pos)) {
					continue;
				}
			} else if (is_near_water) {
				if (!map->is_tile_near_water(tile_pos)) {
					continue;
				}
			}

			if (!this->is_pos_available_for_site_generation(tile_pos, province)) {
				continue;
			}

			map->set_tile_site(tile_pos, site);
			return;
		}
	}
}

bool map_template::is_pos_available_for_site(const QPoint &tile_pos, const province *site_province, const QImage &province_image) const
{
	const QRect map_rect(QPoint(0, 0), this->get_size());
	bool available = true;

	static constexpr int coast_check_range = 0;
	const QRect coast_check_rect(tile_pos - QPoint(coast_check_range, coast_check_range), tile_pos + QPoint(coast_check_range, coast_check_range));

	rect::for_each_point_until(coast_check_rect, [this, &map_rect, &available, site_province, &province_image](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		if (!province_image.isNull()) {
			const province *tile_province = province::try_get_by_color(province_image.pixelColor(rect_pos));
			if (tile_province == nullptr || (tile_province->is_water_zone() && site_province != tile_province)) {
				available = false;
				return true;
			}
		}

		return false;
	});

	static constexpr int province_check_range = 2;
	const QRect province_check_rect(tile_pos - QPoint(province_check_range, province_check_range), tile_pos + QPoint(province_check_range, province_check_range));

	rect::for_each_point_until(province_check_rect, [this, &map_rect, &available, site_province, &province_image](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		if (!province_image.isNull()) {
			const province *tile_province = province::try_get_by_color(province_image.pixelColor(rect_pos));
			if (tile_province != nullptr && !tile_province->is_water_zone() && site_province != tile_province) {
				available = false;
				return true;
			}
		}

		return false;
	});

	if (!available) {
		return false;
	}

	static constexpr int site_check_range = 4;
	const QRect site_check_rect(tile_pos - QPoint(site_check_range, site_check_range), tile_pos + QPoint(site_check_range, site_check_range));

	rect::for_each_point_until(site_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			return false;
		}

		if (this->sites_by_position.contains(rect_pos)) {
			available = false;
			return true;
		}

		return false;
	});

	return available;
}

bool map_template::is_pos_available_for_site_generation(const QPoint &tile_pos, const province *site_province) const
{
	const QRect map_rect(QPoint(0, 0), map::get()->get_size());
	bool available = true;

	static constexpr int coast_check_range = 0;
	const QRect coast_check_rect(tile_pos - QPoint(coast_check_range, coast_check_range), tile_pos + QPoint(coast_check_range, coast_check_range));

	rect::for_each_point_until(coast_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		const province *tile_province = tile->get_province();
		if (tile_province == nullptr || (tile_province->is_water_zone() && site_province != tile_province)) {
			available = false;
			return true;
		}

		return false;
	});

	static constexpr int province_check_range = 2;
	const QRect province_check_rect(tile_pos - QPoint(province_check_range, province_check_range), tile_pos + QPoint(province_check_range, province_check_range));

	rect::for_each_point_until(province_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			available = false;
			return true;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		const province *tile_province = tile->get_province();
		if (tile_province != nullptr && !tile_province->is_water_zone() && site_province != tile_province) {
			available = false;
			return true;
		}

		return false;
	});

	if (!available) {
		return false;
	}

	static constexpr int site_check_range = 4;
	const QRect site_check_rect(tile_pos - QPoint(site_check_range, site_check_range), tile_pos + QPoint(site_check_range, site_check_range));

	rect::for_each_point_until(site_check_rect, [this, &map_rect, &available, site_province](const QPoint &rect_pos) {
		if (!map_rect.contains(rect_pos)) {
			return false;
		}

		const tile *tile = map::get()->get_tile(rect_pos);
		if (tile->get_site() != nullptr) {
			available = false;
			return true;
		}

		return false;
	});

	return available;
}

}
