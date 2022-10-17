#include "metternich.h"

#include "map/map_template.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/map_projection.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/geoshape_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"

namespace metternich {

//map templates must be initialized after sites, as sites add themselves to the world site list in their initialization function, and during map template initialization the sites are then added to the map template's site position map
const std::set<std::string> map_template::database_dependencies = { site::class_identifier };

void map_template::initialize()
{
	const QRect map_rect(QPoint(0, 0), this->get_size());

	for (const site *site : this->get_world()->get_sites()) {
		if (!this->get_georectangle().contains(site->get_geocoordinate())) {
			continue;
		}

		const QPoint tile_pos = this->get_geocoordinate_pos(site->get_geocoordinate());

		if (!map_rect.contains(tile_pos)) {
			continue;
		}

		if (this->sites_by_position.contains(tile_pos)) {
			throw std::runtime_error("Both the sites of \"" + this->sites_by_position.find(tile_pos)->second->get_identifier() + "\" and \"" + site->get_identifier() + "\" occupy the " + point::to_string(tile_pos) + " position in map template \"" + this->get_identifier() + "\".");
		}

		this->sites_by_position[tile_pos] = site;
	}

	data_entry::initialize();
}

QPoint map_template::get_geocoordinate_pos(const geocoordinate &geocoordinate) const
{
	return this->map_projection->geocoordinate_to_point(geocoordinate, this->get_georectangle(), this->get_size());
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

	//write terrain sites
	for (const auto &[tile_pos, site] : this->sites_by_position) {
		if (site->get_terrain_type() == nullptr) {
			continue;
		}

		assert_throw(site->get_type() == site_type::terrain || site->get_type() == site_type::resource);

		if (base_image.pixelColor(tile_pos).alpha() != 0) {
			//ignore already-written pixels
			continue;
		}

		const QColor terrain_color = site->get_terrain_type()->get_color();

		assert_throw(terrain_color.isValid());

		base_image.setPixelColor(tile_pos, terrain_color);
	}

	//write terrain geoshapes
	std::filesystem::path output_filepath = this->get_terrain_image_filepath().filename();
	if (output_filepath.empty()) {
		output_filepath = "terrain.png";
	}

	geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image);

	if (!this->get_province_image_filepath().empty()) {
		//write terrain based on provinces, for pixels without any terrain data set for them
		const QImage province_image(path::to_qstring(this->get_province_image_filepath()));
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

				const terrain_type *terrain = nullptr;
				if (province->is_water_zone()) {
					terrain = defines::get()->get_default_water_zone_terrain();
				} else {
					terrain = defines::get()->get_default_province_terrain();
				}

				assert_throw(terrain != nullptr);

				const QColor terrain_color = terrain->get_color();

				assert_throw(terrain_color.isValid());

				output_image.setPixelColor(x, y, terrain_color);
			}
		}

		output_image.save(path::to_qstring(output_filepath));
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

	geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image);
}

void map_template::apply() const
{
	map *map = map::get();
	map->clear();
	map->set_size(this->get_size());
	map->create_tiles();

	this->apply_terrain();
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

			if (terrain_image.pixelColor(tile_pos).alpha() == 0) {
				continue;
			}

			const terrain_type *terrain = terrain_type::get_by_color(tile_color);
			map->set_tile_terrain(tile_pos, terrain);
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

			if (province_image.pixelColor(tile_pos).alpha() == 0) {
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
