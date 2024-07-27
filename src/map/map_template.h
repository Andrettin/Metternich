#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/province_container.h"
#include "util/fractional_int.h"
#include "util/georectangle.h"
#include "util/point_container.h"

Q_MOC_INCLUDE("map/map_projection.h")
Q_MOC_INCLUDE("map/world.h")

class QGeoShape;

namespace archimedes {
	class map_projection;
}

namespace metternich {

class site;
class world;

class map_template final : public named_data_entry, public data_type<map_template>
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size READ get_size)
	Q_PROPERTY(metternich::world* world MEMBER world)
	Q_PROPERTY(archimedes::map_projection* map_projection MEMBER map_projection)
	Q_PROPERTY(archimedes::decimillesimal_int min_longitude READ get_min_longitude WRITE set_min_longitude)
	Q_PROPERTY(archimedes::decimillesimal_int max_longitude READ get_max_longitude WRITE set_max_longitude)
	Q_PROPERTY(archimedes::decimillesimal_int min_latitude READ get_min_latitude WRITE set_min_latitude)
	Q_PROPERTY(archimedes::decimillesimal_int max_latitude READ get_max_latitude WRITE set_max_latitude)
	Q_PROPERTY(int geocoordinate_x_offset MEMBER geocoordinate_x_offset)
	Q_PROPERTY(std::filesystem::path terrain_image_filepath MEMBER terrain_image_filepath WRITE set_terrain_image_filepath)
	Q_PROPERTY(std::filesystem::path river_image_filepath MEMBER river_image_filepath WRITE set_river_image_filepath)
	Q_PROPERTY(std::filesystem::path border_river_image_filepath MEMBER border_river_image_filepath WRITE set_border_river_image_filepath)
	Q_PROPERTY(std::filesystem::path route_image_filepath MEMBER route_image_filepath WRITE set_route_image_filepath)
	Q_PROPERTY(std::filesystem::path province_image_filepath MEMBER province_image_filepath WRITE set_province_image_filepath)

public:
	using province_geodata_map_type = province_map<std::vector<std::unique_ptr<QGeoShape>>>;

	static constexpr const char class_identifier[] = "map_template";
	static constexpr const char property_class_identifier[] = "metternich::map_template*";
	static constexpr const char database_folder[] = "map_templates";

	static const std::set<std::string> database_dependencies;

	static bool is_site_in_province(const site *site, const province *province, const province_geodata_map_type &province_geodata_map);

	explicit map_template(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const QSize &get_size() const
	{
		return this->size;
	}

	const metternich::world *get_world() const
	{
		return this->world;
	}

	const metternich::map_projection *get_map_projection() const
	{
		return this->map_projection;
	}

	const archimedes::georectangle &get_georectangle() const
	{
		return this->georectangle;
	}

	const decimillesimal_int &get_min_longitude() const
	{
		return this->get_georectangle().get_min_longitude();
	}

	void set_min_longitude(const decimillesimal_int &lon)
	{
		this->georectangle.set_min_longitude(lon);
	}

	const decimillesimal_int &get_max_longitude() const
	{
		return this->get_georectangle().get_max_longitude();
	}

	void set_max_longitude(const decimillesimal_int &lon)
	{
		this->georectangle.set_max_longitude(lon);
	}

	const decimillesimal_int &get_min_latitude() const
	{
		return this->get_georectangle().get_min_latitude();
	}

	void set_min_latitude(const decimillesimal_int &lat)
	{
		this->georectangle.set_min_latitude(lat);
	}

	const decimillesimal_int &get_max_latitude() const
	{
		return this->get_georectangle().get_max_latitude();
	}

	void set_max_latitude(const decimillesimal_int &lat)
	{
		this->georectangle.set_max_latitude(lat);
	}

	QPoint get_geocoordinate_pos(const geocoordinate &geocoordinate) const;
	geocoordinate get_pos_geocoordinate(const QPoint &pos) const;

	const std::filesystem::path &get_terrain_image_filepath() const
	{
		return this->terrain_image_filepath;
	}

	void set_terrain_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_terrain_image();

	const std::filesystem::path &get_river_image_filepath() const
	{
		return this->river_image_filepath;
	}

	void set_river_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_river_image();

	const std::filesystem::path &get_border_river_image_filepath() const
	{
		return this->border_river_image_filepath;
	}

	void set_border_river_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_border_river_image();

	const std::filesystem::path &get_route_image_filepath() const
	{
		return this->route_image_filepath;
	}

	void set_route_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_route_image();

	const std::filesystem::path &get_province_image_filepath() const
	{
		return this->province_image_filepath;
	}

	void set_province_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_province_image();

	void apply() const;
	void apply_terrain() const;
	void apply_rivers() const;
	void apply_border_rivers() const;
	void apply_routes() const;
	void apply_provinces() const;

private:
	QSize size = QSize(0, 0);
	metternich::world *world = nullptr;
	archimedes::map_projection *map_projection = nullptr;
	archimedes::georectangle georectangle = archimedes::georectangle(geocoordinate(geocoordinate::min_longitude, geocoordinate::min_latitude), geocoordinate(geocoordinate::max_longitude, geocoordinate::max_latitude));
	int geocoordinate_x_offset = 0;
	std::filesystem::path terrain_image_filepath;
	std::filesystem::path river_image_filepath;
	std::filesystem::path border_river_image_filepath;
	std::filesystem::path route_image_filepath;
	std::filesystem::path province_image_filepath;
	point_map<const site *> sites_by_position;
};

}
