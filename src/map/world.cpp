#include "metternich.h"

#include "map/world.h"

#include "map/province.h"
#include "map/route.h"
#include "map/site.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "util/geojson_util.h"
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

province_map<std::vector<std::unique_ptr<QGeoShape>>> world::parse_provinces_geojson_folder() const
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
