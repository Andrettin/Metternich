#include "metternich.h"

#include "map/terrain_geodata_map.h"

#include "map/terrain_feature.h"
#include "map/terrain_type.h"

namespace metternich {

bool terrain_geodata_map_compare::operator()(const terrain_geodata_key &terrain_variant, const terrain_geodata_key  &other_terrain_variant) const
{
	const terrain_type *terrain = nullptr;
	const terrain_feature *terrain_feature = nullptr;
	if (std::holds_alternative<const metternich::terrain_feature *>(terrain_variant)) {
		terrain_feature = std::get<const metternich::terrain_feature *>(terrain_variant);
		terrain = terrain_feature->get_terrain_type();
	} else {
		terrain = std::get<const terrain_type *>(terrain_variant);
	}

	const terrain_type *other_terrain = nullptr;
	const metternich::terrain_feature *other_terrain_feature = nullptr;
	if (std::holds_alternative<const metternich::terrain_feature *>(other_terrain_variant)) {
		other_terrain_feature = std::get<const metternich::terrain_feature *>(other_terrain_variant);
		other_terrain = other_terrain_feature->get_terrain_type();
	} else {
		other_terrain = std::get<const terrain_type *>(other_terrain_variant);
	}

	const bool is_water = terrain != nullptr && terrain->is_water();
	const bool is_other_water = other_terrain != nullptr && other_terrain->is_water();
	if (is_water != is_other_water) {
		return is_water; //give priority to water terrain, so that it is written first
	}

	if (terrain == other_terrain && terrain_feature != other_terrain_feature) {
		if (terrain_feature == nullptr || other_terrain_feature == nullptr) {
			return terrain_feature != nullptr;
		}

		return terrain_feature->get_identifier() < other_terrain_feature->get_identifier();
	}

	if (terrain == nullptr || other_terrain == nullptr) {
		return terrain == nullptr;
	}

	return terrain->get_identifier() < other_terrain->get_identifier();
}

}
