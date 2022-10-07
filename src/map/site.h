#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/geocoordinate.h"

namespace metternich {

class terrain_type;
class world;
enum class site_type;

class site final : public named_data_entry, public data_type<site>
{
	Q_OBJECT

	Q_PROPERTY(metternich::world* world MEMBER world)
	Q_PROPERTY(archimedes::geocoordinate geocoordinate MEMBER geocoordinate READ get_geocoordinate)
	Q_PROPERTY(metternich::site_type type MEMBER type READ get_type)
	Q_PROPERTY(metternich::terrain_type* terrain_type MEMBER terrain_type)

public:
	static constexpr const char class_identifier[] = "site";
	static constexpr const char property_class_identifier[] = "metternich::site*";
	static constexpr const char database_folder[] = "sites";

public:
	explicit site(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	const metternich::world *get_world() const
	{
		return this->world;
	}

	const archimedes::geocoordinate &get_geocoordinate() const
	{
		return this->geocoordinate;
	}

	site_type get_type() const
	{
		return this->type;
	}

	const metternich::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
	}

private:
	metternich::world *world = nullptr;
	archimedes::geocoordinate geocoordinate;
	site_type type;
	metternich::terrain_type *terrain_type = nullptr;
};

}
