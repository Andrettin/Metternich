#pragma once

namespace metternich {

class terrain_feature;
class terrain_type;

using terrain_geodata_key = std::variant<const terrain_type *, const terrain_feature *>;

struct terrain_geodata_map_compare
{
	bool operator()(const terrain_geodata_key &terrain_variant, const terrain_geodata_key &other_terrain_variant) const;
};

using terrain_geodata_map = std::map<terrain_geodata_key, std::vector<std::unique_ptr<QGeoShape>>, terrain_geodata_map_compare>;
using terrain_geodata_ptr_map = std::map<terrain_geodata_key, std::vector<const QGeoShape *>, terrain_geodata_map_compare>;

}
