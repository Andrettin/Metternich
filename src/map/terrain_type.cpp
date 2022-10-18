#include "metternich.h"

#include "map/terrain_type.h"

#include "database/database.h"
#include "map/direction.h"
#include "map/terrain_adjacency_type.h"
#include "util/assert_util.h"

namespace metternich {
	
void terrain_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "tiles") {
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

			this->adjacency_tiles[std::move(adjacency)] = std::move(tiles);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void terrain_type::check() const
{
	assert_throw(this->get_color().isValid());
	assert_throw(!this->get_image_filepath().empty());
	assert_throw(std::filesystem::exists(this->get_image_filepath()));
}

void terrain_type::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
