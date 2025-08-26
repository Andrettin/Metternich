#include "metternich.h"

#include "database/defines.h"

#include "character/character_trait_type.h"
#include "country/diplomacy_state.h"
#include "database/database.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "game/character_event.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/province_event.h"
#include "map/direction.h"
#include "map/terrain_adjacency_type.h"
#include "map/tile_image_provider.h"
#include "script/modifier.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/path_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

defines::defines() : min_log_level(log_level::warning)
{
	defines_base::instance = this;

	connect(this, &defines::changed, this, &defines::scaled_tile_size_changed);
	connect(preferences::get(), &preferences::scale_factor_changed, this, &defines::scaled_tile_size_changed);
}

defines::~defines()
{
}

void defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "months_per_turn_from_year") {
		scope.for_each_property([&](const gsml_property &property) {
			const int threshold_year = std::stoi(property.get_key());
			const int months_per_turn = std::stoi(property.get_value());

			this->months_per_turn_from_year[threshold_year] = months_per_turn;
		});
	} else if (tag == "settlement_commodity_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			const int bonus = std::stoi(property.get_value());

			this->settlement_commodity_bonuses[commodity] = bonus;
		});
	} else if (tag == "river_settlement_commodity_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			const int bonus = std::stoi(property.get_value());

			this->river_settlement_commodity_bonuses[commodity] = bonus;
		});
	} else if (tag == "min_character_traits_per_type") {
		scope.for_each_property([&](const gsml_property &property) {
			const character_trait_type type = magic_enum::enum_cast<character_trait_type>(property.get_key()).value();
			const int value = std::stoi(property.get_value());

			this->min_character_traits_per_type[type] = value;
		});
	} else if (tag == "max_character_traits_per_type") {
		scope.for_each_property([&](const gsml_property &property) {
			const character_trait_type type = magic_enum::enum_cast<character_trait_type>(property.get_key()).value();
			const int value = std::stoi(property.get_value());

			this->max_character_traits_per_type[type] = value;
		});
	} else if (tag == "scaled_landholder_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const site>>();
		modifier->process_gsml_data(scope);
		this->scaled_landholder_modifier = std::move(modifier);
	} else if (tag == "diplomacy_state_colors") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const diplomacy_state diplomacy_state = magic_enum::enum_cast<metternich::diplomacy_state>(child_tag).value();
			this->diplomacy_state_colors[diplomacy_state] = child_scope.to_color();
		});
	} else if (tag == "event_trigger_none_random_weights") {
		scope.for_each_property([&](const gsml_property &property) {
			const event_trigger trigger = magic_enum::enum_cast<event_trigger>(property.get_key()).value();
			const int weight = std::stoi(property.get_value());

			this->event_trigger_none_random_weights[trigger] = weight;
		});
	} else if (tag == "province_taxation_per_level") {
		scope.for_each_property([&](const gsml_property &property) {
			const int level = std::stoi(property.get_key());
			dice dice(property.get_value());

			this->province_taxation_per_level[level] = std::move(dice);
		});
	} else if (tag == "river_adjacency_subtiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const int subtile = std::stoi(child_scope.get_tag());
			std::vector<int> subtiles;
			subtiles.push_back(subtile);

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

			this->set_river_adjacency_subtiles(adjacency, subtiles);
		});
	} else if (tag == "rivermouth_adjacency_tiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const int tile = std::stoi(child_scope.get_tag());

			terrain_adjacency adjacency;

			child_scope.for_each_property([&](const gsml_property &property) {
				const direction direction = string_to_direction(property.get_key());
				const terrain_adjacency_type adjacency_type = string_to_terrain_adjacency_type(property.get_value());
				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			});

			this->set_rivermouth_adjacency_tile(adjacency, tile);
		});
	} else if (tag == "route_adjacency_tiles") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const int tile = std::stoi(child_scope.get_tag());

			terrain_adjacency adjacency;

			child_scope.for_each_property([&](const gsml_property &property) {
				const direction direction = string_to_direction(property.get_key());
				const terrain_adjacency_type adjacency_type = string_to_terrain_adjacency_type(property.get_value());
				adjacency.set_direction_adjacency_type(direction, adjacency_type);
			});

			this->set_route_adjacency_tile(adjacency, tile);
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

void defines::initialize()
{
	log::min_log_level = this->get_min_log_level();

	for (const auto &[event_trigger, random_weight] : this->get_event_trigger_none_random_weights()) {
		character_event::add_trigger_none_random_weight(event_trigger, random_weight);
		country_event::add_trigger_none_random_weight(event_trigger, random_weight);
		province_event::add_trigger_none_random_weight(event_trigger, random_weight);
	}

	tile_image_provider::get()->load_image("borders/province_border");
	tile_image_provider::get()->load_image("river/0");
	tile_image_provider::get()->load_image("rivermouth/0");
}

void defines::check() const
{
	assert_throw(this->get_great_power_commodity_demand_divisor() > 0);
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

void defines::set_river_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_river_image_filepath()) {
		return;
	}

	this->river_image_filepath = database::get()->get_graphics_filepath(filepath);
}

void defines::set_rivermouth_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_rivermouth_image_filepath()) {
		return;
	}

	this->rivermouth_image_filepath = database::get()->get_graphics_filepath(filepath);
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

const dice &defines::get_province_taxation_for_level(const int level) const
{
	const auto find_iterator = this->province_taxation_per_level.find(level);
	assert_throw(find_iterator != this->province_taxation_per_level.end());
	return find_iterator->second;
}

const std::vector<int> &defines::get_river_adjacency_subtiles(const terrain_adjacency &adjacency) const
{
	const auto find_iterator = this->river_adjacency_subtiles.find(adjacency);
	if (find_iterator != this->river_adjacency_subtiles.end()) {
		return find_iterator->second;
	}

	log::log_error(std::format("Failed to get river adjacency tile for adjacency:\n{}", adjacency.to_string()));

	static const std::vector<int> empty_vector;
	return empty_vector;
}

void defines::set_river_adjacency_subtiles(const terrain_adjacency &adjacency, const std::vector<int> &subtiles)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_river_adjacency_subtiles(same_adjacency, subtiles);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_river_adjacency_subtiles(other_adjacency, subtiles);
			return;
		}
	}

	const auto result = this->river_adjacency_subtiles.insert_or_assign(adjacency, subtiles);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_log(result.second);
}

int defines::get_rivermouth_adjacency_tile(const terrain_adjacency &adjacency) const
{
	const auto find_iterator = this->rivermouth_adjacency_tiles.find(adjacency);
	if (find_iterator != this->rivermouth_adjacency_tiles.end()) {
		return find_iterator->second;
	}

	log::log_error("Failed to get rivermouth adjacency tile for adjacency:\n" + adjacency.to_string());

	return -1;
}

void defines::set_rivermouth_adjacency_tile(const terrain_adjacency &adjacency, const int tile)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_rivermouth_adjacency_tile(same_adjacency, tile);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_rivermouth_adjacency_tile(other_adjacency, tile);
			return;
		}
	}

	const auto result = this->rivermouth_adjacency_tiles.insert_or_assign(adjacency, tile);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_throw(result.second);
}

int defines::get_route_adjacency_tile(const terrain_adjacency &adjacency) const
{
	const auto find_iterator = this->route_adjacency_tiles.find(adjacency);
	if (find_iterator != this->route_adjacency_tiles.end()) {
		return find_iterator->second;
	}

	log::log_error("Failed to get route adjacency tile for adjacency:\n" + adjacency.to_string());

	return -1;
}

void defines::set_route_adjacency_tile(const terrain_adjacency &adjacency, const int tile)
{
	const std::array<terrain_adjacency_type, terrain_adjacency::direction_count> &adjacency_data = adjacency.get_data();

	//convert "any" to both "same" and "other" adjacency types
	//this works recursively: we change the adjacency type for only one direction, but each recursive call will change for other further direction, until no direction is set to "any"
	for (size_t i = 0; i < adjacency_data.size(); ++i) {
		if (adjacency_data[i] == terrain_adjacency_type::any) {
			terrain_adjacency same_adjacency = adjacency;
			same_adjacency.get_data()[i] = terrain_adjacency_type::same;
			this->set_route_adjacency_tile(same_adjacency, tile);

			terrain_adjacency other_adjacency = adjacency;
			other_adjacency.get_data()[i] = terrain_adjacency_type::other;
			this->set_route_adjacency_tile(other_adjacency, tile);
			return;
		}
	}

	const auto result = this->route_adjacency_tiles.insert_or_assign(adjacency, tile);

	//if this is false, that means there was already a definition for the same adjacency data
	//multiple adjacency definitions with the same adjacency data is an error
	assert_log(result.second);
}

}
