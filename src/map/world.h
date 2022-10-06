#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "map/province_container.h"

namespace metternich {

class world final : public named_data_entry, public data_type<world>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "world";
	static constexpr const char property_class_identifier[] = "metternich::world*";
	static constexpr const char database_folder[] = "worlds";
	static constexpr const char provinces_map_folder[] = "provinces";

	explicit world(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	std::vector<QVariantList> parse_geojson_folder(const std::string_view &folder) const;
	province_map<std::vector<std::unique_ptr<QGeoShape>>> parse_provinces_geojson_folder() const;
};

}
