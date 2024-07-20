#include "metternich.h"

#include "map/world.h"

#include "map/equirectangular_map_projection.h"
#include "map/province.h"
#include "map/route.h"
#include "map/site.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/geojson_util.h"
#include "util/georectangle.h"
#include "util/geoshape_util.h"
#include "util/number_util.h"
#include "util/path_util.h"
#include "util/vector_util.h"

namespace metternich {

void world::add_site(const site *site)
{
	this->sites.push_back(site);

	const geocoordinate site_geocoordinate = site->get_geocoordinate();
	if (!site_geocoordinate.is_null()) {
		if (this->sites_by_geocoordinate.contains(site_geocoordinate)) {
			throw std::runtime_error(std::format("Both the sites of \"{}\" and \"{}\" occupy the {} geocoordinate in world \"{}\".", this->sites_by_geocoordinate.find(site_geocoordinate)->second->get_identifier(), site->get_identifier(), site_geocoordinate.to_string(), this->get_identifier()));
		} else {
			this->sites_by_geocoordinate[site_geocoordinate] = site;
		}
	}
}

std::vector<QVariantList> world::parse_geojson_folder(const std::string_view &folder) const
{
	std::vector<QVariantList> geojson_data_list;

	for (const std::filesystem::path &path : database::get()->get_maps_paths()) {
		const std::filesystem::path map_path = path / this->get_identifier() / folder;

		if (!std::filesystem::exists(map_path)) {
			continue;
		}

		std::vector<QVariantList> folder_geojson_data_list = geojson::parse_folder(map_path);
		vector::merge(geojson_data_list, std::move(folder_geojson_data_list));
	}

	return geojson_data_list;
}

terrain_geodata_map world::parse_terrain_geojson_folder() const
{
	const std::vector<QVariantList> geojson_data_list = this->parse_geojson_folder(world::terrain_map_folder);

	return geojson::create_geodata_map<terrain_geodata_map>(geojson_data_list, [](const QVariantMap &properties) -> terrain_geodata_map::key_type {
		if (properties.contains("terrain_feature")) {
			const QString terrain_feature_identifier = properties.value("terrain_feature").toString();
			return terrain_feature::get(terrain_feature_identifier.toStdString());
		} else {
			const QString terrain_type_identifier = properties.value("terrain_type").toString();
			return terrain_type::get(terrain_type_identifier.toStdString());
		}
	}, nullptr);
}

route_map<std::vector<std::unique_ptr<QGeoShape>>> world::parse_routes_geojson_folder() const
{
	using route_geodata_map = route_map<std::vector<std::unique_ptr<QGeoShape>>>;

	const std::vector<QVariantList> geojson_data_list = this->parse_geojson_folder(world::routes_map_folder);

	return geojson::create_geodata_map<route_geodata_map>(geojson_data_list, [](const QVariantMap &properties) -> route_geodata_map::key_type {
		const QString route_identifier = properties.value("route").toString();
		const route *route = route::get(route_identifier.toStdString());
		return route;
	}, nullptr);
}

void world::set_province_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_province_image_filepath()) {
		return;
	}

	this->province_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void world::write_province_image(const double min_geoshape_width, const double max_geoshape_width)
{
	try {
		province_geodata_map_type province_geodata_map = this->parse_provinces_geojson_folder();

		color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

		for (auto &[province, geoshapes] : province_geodata_map) {
			const QColor color = province->get_color();
			if (!color.isValid()) {
				throw std::runtime_error("Province \"" + province->get_identifier() + "\" has no valid color.");
			}

			for (size_t i = 0; i < geoshapes.size();) {
				const QGeoRectangle bounding_georectangle = geoshapes[i]->boundingGeoRectangle();
				if (bounding_georectangle.width() < min_geoshape_width || bounding_georectangle.width() > max_geoshape_width) {
					geoshapes.erase(geoshapes.begin() + i);
				} else {
					++i;
				}
			}

			vector::merge(geodata_map[color], std::move(geoshapes));
		}

		const equirectangular_map_projection *map_projection = equirectangular_map_projection::get();

		static constexpr archimedes::georectangle georectangle = georectangle::get_global_georectangle();

		static constexpr int geocoordinate_size_multiplier = 25;
		static constexpr QSize map_size(geocoordinate::longitude_size * geocoordinate_size_multiplier, geocoordinate::latitude_size * geocoordinate_size_multiplier);

		map_projection->validate_area(georectangle, map_size);

		QImage base_image;

		if (!this->get_province_image_filepath().empty()) {
			base_image = QImage(path::to_qstring(this->get_province_image_filepath()));
			assert_throw(!base_image.isNull());
		}

		std::filesystem::path output_filepath = this->get_province_image_filepath().filename();
		if (output_filepath.empty()) {
			output_filepath = "provinces.png";
		}

		geoshape::write_image(output_filepath, geodata_map, georectangle, map_size, map_projection, base_image, 0);
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

world::province_geodata_map_type world::parse_provinces_geojson_folder() const
{
	using province_geodata_map = province_map<std::vector<std::unique_ptr<QGeoShape>>>;

	const std::vector<QVariantList> geojson_data_list = this->parse_geojson_folder(world::provinces_map_folder);

	return geojson::create_geodata_map<province_geodata_map>(geojson_data_list, [](const QVariantMap &properties) -> province_geodata_map::key_type {
		const QString province_identifier = properties.value("province").toString();
		const province *province = province::get(province_identifier.toStdString());
		return province;
	}, nullptr);
}

}
