#include "metternich.h"

#include "map/terrain_type.h"

#include "database/database.h"
#include "database/defines.h"
#include "map/direction.h"
#include "map/elevation_type.h"
#include "map/forestation_type.h"
#include "map/moisture_type.h"
#include "map/terrain_adjacency_type.h"
#include "map/temperature_type.h"
#include "map/tile_image_provider.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"

namespace metternich {

terrain_type *terrain_type::get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::moisture_type moisture_type, const metternich::forestation_type forestation_type)
{
	terrain_type *terrain_type = terrain_type::try_get_by_biome(elevation_type, temperature_type, moisture_type, forestation_type);

	if (terrain_type == nullptr) {
		throw std::runtime_error("No terrain type found for " + enum_converter<metternich::elevation_type>::to_string(elevation_type) + ", " + enum_converter<metternich::temperature_type>::to_string(temperature_type) + ", " + enum_converter<metternich::moisture_type>::to_string(moisture_type) + ", " + enum_converter<metternich::forestation_type>::to_string(forestation_type) + " biome.");
	}

	return terrain_type;
}

terrain_type *terrain_type::try_get_by_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::moisture_type moisture_type, const metternich::forestation_type forestation_type)
{
	const auto find_iterator = terrain_type::terrain_types_by_biome.find(elevation_type);
	if (find_iterator == terrain_type::terrain_types_by_biome.end()) {
		return nullptr;
	}

	const auto find_iterator_2 = find_iterator->second.find(temperature_type);
	if (find_iterator_2 == find_iterator->second.end()) {
		if (temperature_type != temperature_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type::none, moisture_type, forestation_type);
		}

		return nullptr;
	}

	const auto find_iterator_3 = find_iterator_2->second.find(moisture_type);
	if (find_iterator_3 == find_iterator_2->second.end()) {
		if (moisture_type != moisture_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type, moisture_type::none, forestation_type);
		}

		if (temperature_type != temperature_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type::none, moisture_type, forestation_type);
		}

		return nullptr;
	}

	const auto find_iterator_4 = find_iterator_3->second.find(forestation_type);
	if (find_iterator_4 == find_iterator_3->second.end()) {
		if (forestation_type != forestation_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type, moisture_type, forestation_type::none);
		}

		if (moisture_type != moisture_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type, moisture_type::none, forestation_type);
		}

		if (temperature_type != temperature_type::none) {
			return terrain_type::try_get_by_biome(elevation_type, temperature_type::none, moisture_type, forestation_type);
		}

		return nullptr;
	}

	return find_iterator_4->second;
}

terrain_type::terrain_type(const std::string &identifier)
	: named_data_entry(identifier), elevation_type(elevation_type::none), temperature_type(temperature_type::none), moisture_type(moisture_type::none), forestation_type(forestation_type::none)
{
}

	
void terrain_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "fallback_terrains") {
		for (const std::string &value : values) {
			this->fallback_terrains.push_back(terrain_type::get(value));
		}
	} else if (tag == "tiles") {
		for (const std::string &value : values) {
			this->tiles.push_back(std::stoi(value));
		}
	} else if (tag == "adjacency_tiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			std::vector<int> tiles;
			tiles.push_back(std::stoi(child_scope.get_tag()));

			terrain_adjacency adjacency;

			child_scope.for_each_property([&](const gsml_property &property) {
				const direction direction = string_to_direction(property.get_key());
				const terrain_adjacency_type adjacency_type = string_to_terrain_adjacency_type(property.get_value());
				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			});

			child_scope.for_each_child([&](const gsml_data &grandchild_scope) {
				if (grandchild_scope.get_tag() == "variations") {
					for (const std::string &value : grandchild_scope.get_values()) {
						tiles.push_back(std::stoi(value));
					}
				} else {
					assert_throw(false);
				}
			});

			this->set_adjacency_tiles(adjacency, tiles);
		});
	} else if (tag == "subtiles") {
		for (const std::string &value : values) {
			this->subtiles.push_back(std::stoi(value));
		}
	} else if (tag == "adjacency_subtiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			std::vector<int> subtiles;
			subtiles.push_back(std::stoi(child_scope.get_tag()));

			terrain_adjacency adjacency;

			child_scope.for_each_property([&](const gsml_property &property) {
				const direction direction = string_to_direction(property.get_key());
				const terrain_adjacency_type adjacency_type = string_to_terrain_adjacency_type(property.get_value());
				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			});

			child_scope.for_each_child([&](const gsml_data &grandchild_scope) {
				if (grandchild_scope.get_tag() == "variations") {
					for (const std::string &value : grandchild_scope.get_values()) {
						subtiles.push_back(std::stoi(value));
					}
				} else {
					assert_throw(false);
				}
			});

			this->set_adjacency_subtiles(adjacency, subtiles);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void terrain_type::initialize()
{
	if (this->get_elevation_type() != elevation_type::none) {
		assign_to_biome(this->get_elevation_type(), this->get_temperature_type(), this->get_moisture_type(), this->get_forestation_type());
	}

	tile_image_provider::get()->load_image("terrain/" + this->get_identifier() + "/0");

	named_data_entry::initialize();
}

void terrain_type::check() const
{
	assert_throw(this->get_color().isValid());

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Terrain type \"{}\" has no icon.", this->get_identifier()));
	}

	assert_throw(!this->get_image_filepath().empty());
	assert_throw(std::filesystem::exists(this->get_image_filepath()));

	if (this->has_adjacency_tiles()) {
		//check whether the terrain type has support for all possible adjacencies
		std::vector<terrain_adjacency> possible_adjacencies;
		possible_adjacencies.emplace_back();

		for (size_t i = 0; i < terrain_adjacency::direction_count; ++i) {
			for (const terrain_adjacency &adjacency : possible_adjacencies) {
				terrain_adjacency other_adjacency = adjacency;
				other_adjacency.get_data()[i] = terrain_adjacency_type::other;
				possible_adjacencies.push_back(std::move(other_adjacency));
			}
		}

		for (const terrain_adjacency &adjacency : possible_adjacencies) {
			if (!this->adjacency_tiles.contains(adjacency)) {
				throw std::runtime_error(std::format("No tiles provided for the adjacency for the \"{}\" terrain type:\n{}", this->get_identifier(), adjacency.to_string()));
			}
		}
	}
}

void terrain_type::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

void terrain_type::assign_to_biome(const metternich::elevation_type elevation_type, const metternich::temperature_type temperature_type, const metternich::moisture_type moisture_type, const metternich::forestation_type forestation_type)
{
	assert_throw(elevation_type != elevation_type::none);
	assert_throw(terrain_type::terrain_types_by_biome[elevation_type][temperature_type][moisture_type][forestation_type] == nullptr);

	terrain_type::terrain_types_by_biome[elevation_type][temperature_type][moisture_type][forestation_type] = this;
}

void terrain_type::set_adjacency_tiles(const terrain_adjacency &adjacency, const std::vector<int> &tiles)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_adjacency_tiles(same_adjacency, tiles);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_adjacency_tiles(other_adjacency, tiles);
			return;
		}
	}

	const auto result = this->adjacency_tiles.insert_or_assign(adjacency, tiles);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_throw(result.second); 
}

void terrain_type::set_adjacency_subtiles(const terrain_adjacency &adjacency, const std::vector<int> &subtiles)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_adjacency_subtiles(same_adjacency, subtiles);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_adjacency_subtiles(other_adjacency, subtiles);
			return;
		}
	}

	const auto result = this->adjacency_subtiles.insert_or_assign(adjacency, subtiles);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_throw(result.second); 
}

}
