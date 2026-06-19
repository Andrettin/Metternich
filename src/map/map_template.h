#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/province_container.h"
#include "util/decimillesimal_int.h"
#include "util/georectangle.h"
#include "util/point_container.h"

Q_MOC_INCLUDE("map/map_projection.h")
Q_MOC_INCLUDE("map/world.h")

class QGeoShape;

namespace archimedes {
	class map_projection;
}

namespace metternich {

class province;
class region;
class site;
class world;

class map_template final : public named_data_entry, public data_type<map_template>
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size READ get_size)
	Q_PROPERTY(const metternich::world* world MEMBER world READ get_world NOTIFY changed)
	Q_PROPERTY(bool universe MEMBER universe READ is_universe NOTIFY changed)
	Q_PROPERTY(archimedes::map_projection* map_projection MEMBER map_projection)
	Q_PROPERTY(archimedes::decimillesimal_int min_longitude READ get_min_longitude WRITE set_min_longitude)
	Q_PROPERTY(archimedes::decimillesimal_int max_longitude READ get_max_longitude WRITE set_max_longitude)
	Q_PROPERTY(archimedes::decimillesimal_int min_latitude READ get_min_latitude WRITE set_min_latitude)
	Q_PROPERTY(archimedes::decimillesimal_int max_latitude READ get_max_latitude WRITE set_max_latitude)
	Q_PROPERTY(int geocoordinate_x_offset MEMBER geocoordinate_x_offset READ get_geocoordinate_x_offset NOTIFY changed)
	Q_PROPERTY(const metternich::map_template* main_map_template MEMBER main_map_template READ get_main_map_template NOTIFY changed)
	Q_PROPERTY(QPoint map_start_pos MEMBER map_start_pos READ get_map_start_pos NOTIFY changed)
	Q_PROPERTY(std::filesystem::path province_image_filepath MEMBER province_image_filepath WRITE set_province_image_filepath)
	Q_PROPERTY(bool randomly_generated MEMBER randomly_generated READ is_randomly_generated NOTIFY changed)
	Q_PROPERTY(bool province_post_processing_enabled MEMBER province_post_processing_enabled READ is_province_post_processing_enabled NOTIFY changed)
	Q_PROPERTY(int land_percent MEMBER land_percent READ get_land_percent NOTIFY changed)
	Q_PROPERTY(int steepness MEMBER steepness READ get_steepness NOTIFY changed)
	Q_PROPERTY(int average_temperature MEMBER average_temperature READ get_average_temperature NOTIFY changed)
	Q_PROPERTY(bool separate_poles MEMBER separate_poles READ are_poles_separate NOTIFY changed)
	Q_PROPERTY(int pole_flattening MEMBER pole_flattening READ get_pole_flattening NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int minimap_tile_scale MEMBER minimap_tile_scale READ get_minimap_tile_scale NOTIFY changed)
	Q_PROPERTY(archimedes::decimillesimal_int diplomatic_map_tile_scale MEMBER diplomatic_map_tile_scale READ get_diplomatic_map_tile_scale NOTIFY changed)
	Q_PROPERTY(bool update_province_image MEMBER update_province_image NOTIFY changed)

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

	virtual void process_gsml_scope(const gsml_data &scope) override;
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

	bool is_universe() const
	{
		return this->universe;
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

	int get_geocoordinate_x_offset() const
	{
		return this->geocoordinate_x_offset;
	}

	QPoint get_geocoordinate_pos(const geocoordinate &geocoordinate) const;
	geocoordinate get_pos_geocoordinate(const QPoint &pos) const;

	const map_template *get_main_map_template() const
	{
		return this->main_map_template;
	}

	const QPoint &get_map_start_pos() const
	{
		return this->map_start_pos;
	}

	Q_INVOKABLE void write_terrain_image();

	const std::filesystem::path &get_province_image_filepath() const
	{
		return this->province_image_filepath;
	}

	void set_province_image_filepath(const std::filesystem::path &filepath);
	Q_INVOKABLE void write_province_image();

	bool is_randomly_generated() const
	{
		return this->randomly_generated;
	}

	bool is_province_post_processing_enabled() const
	{
		return this->province_post_processing_enabled;
	}

	int get_land_percent() const
	{
		return this->land_percent;
	}

	int get_steepness() const
	{
		return this->steepness;
	}

	int get_average_temperature() const
	{
		return this->average_temperature;
	}

	bool are_poles_separate() const
	{
		return this->separate_poles;
	}

	int get_pole_flattening() const
	{
		return this->pole_flattening;
	}

	const decimillesimal_int &get_minimap_tile_scale() const
	{
		return this->minimap_tile_scale;
	}

	const decimillesimal_int &get_diplomatic_map_tile_scale() const
	{
		return this->diplomatic_map_tile_scale;
	}

	[[nodiscard]] QCoro::Task<void> apply() const;
	[[nodiscard]] QCoro::Task<void> apply_provinces() const;
	std::vector<std::pair<QPoint, province *>> apply_provinces_for_map_line(const int y, const std::span<const QRgb> &province_image_data) const;
	void generate_additional_sites() const;
	void generate_site(const site *site) const;

	bool is_pos_available_for_site(const QPoint &tile_pos, const province *site_province, const QImage &province_image) const;
	bool is_pos_available_for_site_generation(const QPoint &tile_pos, const province *site_province) const;

	bool is_province_ignored(const province *province) const;

	const point_map<const site *> &get_sites_by_position() const
	{
		return this->sites_by_position;
	}

signals:
	void changed();

private:
	QSize size = QSize(0, 0);
	const metternich::world *world = nullptr;
	bool universe = false;
	archimedes::map_projection *map_projection = nullptr;
	archimedes::georectangle georectangle = archimedes::georectangle(geocoordinate(geocoordinate::min_longitude, geocoordinate::min_latitude), geocoordinate(geocoordinate::max_longitude, geocoordinate::max_latitude));
	int geocoordinate_x_offset = 0;
	const map_template *main_map_template = nullptr;
	QPoint map_start_pos; //the start position for the part of the map that is actually applied
	std::filesystem::path province_image_filepath;
	bool randomly_generated = false;
	bool province_post_processing_enabled = true;
	int land_percent = 30;
	int steepness = 30;
	int average_temperature = 50;
	bool separate_poles = false; //whether the poles are ensured to be separate continents
	int pole_flattening = 0;
	decimillesimal_int minimap_tile_scale = decimillesimal_int(0);
	decimillesimal_int diplomatic_map_tile_scale = decimillesimal_int(0);
	std::unordered_set<const province *> ignored_provinces;
	std::unordered_set<const region *> ignored_regions;
	point_map<const site *> sites_by_position;
	bool update_province_image = false;
};

}
