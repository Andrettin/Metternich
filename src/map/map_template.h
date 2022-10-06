#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/fractional_int.h"
#include "util/georectangle.h"

namespace archimedes {
	class map_projection;
}

namespace metternich {

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
	Q_PROPERTY(std::filesystem::path province_image_filepath MEMBER province_image_filepath WRITE set_province_image_filepath)

public:
	static constexpr const char class_identifier[] = "map_template";
	static constexpr const char property_class_identifier[] = "metternich::map_template*";
	static constexpr const char database_folder[] = "map_templates";

	explicit map_template(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const QSize &get_size() const
	{
		return this->size;
	}

	const metternich::world *get_world() const
	{
		return this->world;
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

	const std::filesystem::path &get_province_image_filepath() const
	{
		return this->province_image_filepath;
	}

	void set_province_image_filepath(const std::filesystem::path &filepath);
	void write_province_image();

private:
	QSize size = QSize(0, 0);
	metternich::world *world = nullptr;
	archimedes::map_projection *map_projection = nullptr;
	archimedes::georectangle georectangle = archimedes::georectangle(geocoordinate(geocoordinate::min_longitude, geocoordinate::min_latitude), geocoordinate(geocoordinate::max_longitude, geocoordinate::max_latitude));
	std::filesystem::path province_image_filepath;
};

}
