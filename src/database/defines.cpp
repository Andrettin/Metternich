#include "metternich.h"

#include "database/defines.h"

#include "database/database.h"
#include "database/preferences.h"
#include "map/direction.h"
#include "map/terrain_adjacency_type.h"
#include "util/assert_util.h"
#include "util/path_util.h"

namespace metternich {

defines::defines()
{
	connect(this, &defines::changed, this, &defines::scaled_tile_size_changed);
	connect(preferences::get(), &preferences::scale_factor_changed, this, &defines::scaled_tile_size_changed);
}

void defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "river_adjacency_tiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const int tile = std::stoi(child_scope.get_tag());

			terrain_adjacency adjacency;

			child_scope.for_each_property([&](const gsml_property &property) {
				const direction direction = string_to_direction(property.get_key());
				const terrain_adjacency_type adjacency_type = string_to_terrain_adjacency_type(property.get_value());
				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			});

			this->set_river_adjacency_tile(adjacency, tile);
		});
	} else {
		database::get()->process_gsml_scope_for_object(this, scope);
	}
}

QSize defines::get_scaled_tile_size() const
{
	return this->get_tile_size() * preferences::get()->get_scale_factor();
}

int defines::get_scaled_tile_width() const
{
	return (this->get_tile_width() * preferences::get()->get_scale_factor()).to_int();
}

int defines::get_scaled_tile_height() const
{
	return (this->get_tile_height() * preferences::get()->get_scale_factor()).to_int();
}

void defines::set_default_settlement_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_default_settlement_image_filepath()) {
		return;
	}

	this->default_settlement_image_filepath = database::get()->get_graphics_filepath(filepath);
}

void defines::set_province_border_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_province_border_image_filepath()) {
		return;
	}

	this->province_border_image_filepath = database::get()->get_graphics_filepath(filepath);
}

QString defines::get_default_menu_background_filepath_qstring() const
{
	return path::to_qstring(this->default_menu_background_filepath);
}

void defines::set_default_menu_background_filepath(const std::filesystem::path &filepath)
{
	this->default_menu_background_filepath = database::get()->get_graphics_filepath(filepath);
}

void defines::set_river_adjacency_tile(const terrain_adjacency &adjacency, const int tile)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_river_adjacency_tile(same_adjacency, tile);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_river_adjacency_tile(other_adjacency, tile);
			return;
		}
	}

	const auto result = this->river_adjacency_tiles.insert_or_assign(adjacency, tile);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_throw(result.second);
}

}
