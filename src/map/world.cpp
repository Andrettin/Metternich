#include "metternich.h"

#include "world.h"

#include "map/province.h"
#include "util/geojson_util.h"
#include "util/vector_util.h"

namespace metternich {
	
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
