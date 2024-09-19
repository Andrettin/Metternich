#include "metternich.h"

#include "infrastructure/improvement.h"

#include "economy/commodity.h"
#include "economy/resource.h"
#include "infrastructure/improvement_slot.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_image_provider.h"
#include "population/population_type.h"
#include "population/profession.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
improvement::improvement(const std::string &identifier) : named_data_entry(identifier)
{
}

improvement::~improvement()
{
}

void improvement::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "terrain_image_filepaths") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->terrain_image_filepaths[terrain_type::get(key)] = database::get()->get_graphics_path(this->get_module()) / value;
		});
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		database::process_gsml_data(this->modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void improvement::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_improvement(this);
	}

	if (this->get_resource() != nullptr) {
		this->resource->add_improvement(this);
	}

	if (!this->get_image_filepath().empty()) {
		tile_image_provider::get()->load_image("improvement/" + this->get_identifier() + "/0");
	}

	for (const auto &[terrain, filepath] : this->terrain_image_filepaths) {
		tile_image_provider::get()->load_image("improvement/" + this->get_identifier() + "/" + terrain->get_identifier() + "/0");
	}

	this->calculate_level();

	named_data_entry::initialize();
}

void improvement::check() const
{
	if (this->get_slot() == improvement_slot::none) {
		throw std::runtime_error(std::format("Improvement \"{}\" has no slot.", this->get_identifier()));
	}

	if (this->get_resource() != nullptr && this->get_slot() != improvement_slot::resource) {
		throw std::runtime_error(std::format("Improvement \"{}\" has a resource, but is not a resource improvement.", this->get_identifier()));
	}

	if (this->is_ruins() && this->get_slot() != improvement_slot::main) {
		throw std::runtime_error(std::format("Improvement \"{}\" is ruins, but is not a main improvement.", this->get_identifier()));
	}

	if (this->get_output_commodity() != nullptr && this->get_output_multiplier() == 0) {
		throw std::runtime_error(std::format("Improvement \"{}\" has an output commodity, but no output multiplier.", this->get_identifier()));
	}

	if (this->get_output_multiplier() > 0 && this->get_output_commodity() == nullptr) {
		throw std::runtime_error(std::format("Improvement \"{}\" has an output multiplier, but no output commodity.", this->get_identifier()));
	}

	if (this->get_employment_profession() != nullptr && this->get_employment_capacity() == 0) {
		throw std::runtime_error(std::format("Improvement \"{}\" has an employment profession, but no employment capacity.", this->get_identifier()));
	}

	if (this->get_employment_capacity() > 0 && this->get_employment_profession() == nullptr) {
		throw std::runtime_error(std::format("Improvement \"{}\" has an employment capacity, but no employment profession.", this->get_identifier()));
	}

	if ((this->get_slot() == improvement_slot::main || this->get_slot() == improvement_slot::resource) && this->get_image_filepath().empty()) {
		throw std::runtime_error(std::format("Main or resource improvement \"{}\" has no image filepath.", this->get_identifier()));
	}

	for (const auto &[terrain, filepath] : this->terrain_image_filepaths) {
		assert_throw(vector::contains(this->get_terrain_types(), terrain) || vector::contains(this->get_resource()->get_terrain_types(), terrain));
	}

	if ((this->get_slot() == improvement_slot::resource || this->get_slot() == improvement_slot::depot || this->get_slot() == improvement_slot::port) && this->icon == nullptr) {
		throw std::runtime_error(std::format("Non-main improvement \"{}\" has no icon.", this->get_identifier()));
	}
}

void improvement::calculate_level()
{
	if (this->required_improvement != nullptr) {
		if (this->required_improvement->get_level() == 0) {
			this->required_improvement->initialize();
		}

		this->level = this->required_improvement->get_level() + 1;
	} else {
		this->level = 1;
	}
}

void improvement::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

const commodity *improvement::get_output_commodity() const
{
	if (this->get_employment_profession() != nullptr) {
		return this->get_employment_profession()->get_output_commodity();
	}

	if (this->get_resource() != nullptr) {
		return this->get_resource()->get_commodity();
	}

	return nullptr;
}

bool improvement::is_buildable_on_site(const site *site) const
{
	const site_game_data *site_game_data = site->get_game_data();

	switch (this->get_slot()) {
		case improvement_slot::main:
			if (site->is_settlement()) {
				return false;
			}
			break;
		case improvement_slot::depot:
			if (!site_game_data->has_route()) {
				return false;
			}
			break;
		case improvement_slot::port:
			if (!site_game_data->is_near_water()) {
				return false;
			}
			break;
		default:
			break;
	}

	if (this->get_resource() != nullptr && this->get_resource() != site_game_data->get_resource()) {
		return false;
	}

	const improvement *current_improvement = site_game_data->get_improvement(this->get_slot());

	if (this->get_required_improvement() != nullptr && current_improvement != this->get_required_improvement()) {
		return false;
	}

	if (current_improvement != nullptr) {
		if (this == current_improvement) {
			return false;
		}

		if (this->get_output_multiplier() < current_improvement->get_output_multiplier()) {
			return false;
		}

		if (this->get_level() < current_improvement->get_level()) {
			return false;
		}

		if (this->get_output_multiplier() == current_improvement->get_output_multiplier() && this->get_level() == current_improvement->get_level()) {
			//the improvement must be better in some way
			return false;
		}
	}

	return true;
}

}
