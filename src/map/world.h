#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/province_container.h"
#include "map/route_container.h"
#include "map/terrain_geodata_map.h"

namespace metternich {

class site;

class world final : public named_data_entry, public data_type<world>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path province_image_filepath MEMBER province_image_filepath WRITE set_province_image_filepath)

public:
	using province_geodata_map_type = province_map<std::vector<std::unique_ptr<QGeoShape>>>;

	static constexpr const char class_identifier[] = "world";
	static constexpr const char property_class_identifier[] = "metternich::world*";
	static constexpr const char database_folder[] = "worlds";
	static constexpr const char terrain_map_folder[] = "terrain";
	static constexpr const char routes_map_folder[] = "routes";
	static constexpr const char provinces_map_folder[] = "provinces";

	explicit world(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	void add_site(const site *site);

	std::vector<QVariantList> parse_geojson_folder(const std::string_view &folder) const;
	terrain_geodata_map parse_terrain_geojson_folder() const;
	route_map<std::vector<std::unique_ptr<QGeoShape>>> parse_routes_geojson_folder() const;

	const std::filesystem::path &get_province_image_filepath() const
	{
		return this->province_image_filepath;
	}

	void set_province_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_province_image(const double min_geoshape_width, const double max_geoshape_width);
	province_geodata_map_type parse_provinces_geojson_folder() const;

private:
	std::filesystem::path province_image_filepath;
	std::vector<const site *> sites;
	std::map<geocoordinate, const site *> sites_by_geocoordinate;
};

}
