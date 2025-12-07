#include "metternich.h"

#include "infrastructure/improvement.h"

#include "database/database.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "infrastructure/improvement_slot.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_image_provider.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "script/condition/and_condition.h"
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

	if (tag == "resources") {
		for (const std::string &value : values) {
			resource *resource = resource::get(value);
			resource->add_improvement(this);
			this->resources.push_back(resource);
		}
	} else if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "terrain_image_filepaths") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->terrain_image_filepaths[terrain_type::get(key)] = database::get()->get_graphics_path(this->get_module()) / value;
		});
	} else if (tag == "population_classes") {
		for (const std::string &value : values) {
			const population_class *population_class = population_class::get(value);
			this->population_classes.push_back(population_class);
		}
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "free_on_start_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->free_on_start_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->country_modifier->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void improvement::initialize()
{
	if (this->required_improvement != nullptr) {
		this->required_improvement->requiring_improvements.push_back(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_improvement(this);
	}

	if (!this->get_image_filepath().empty()) {
		QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
			co_await tile_image_provider::get()->load_image("improvement/" + this->get_identifier() + "/0");
		});
	}

	for (const auto &[terrain, filepath] : this->terrain_image_filepaths) {
		QTimer::singleShot(0, [this, terrain]() -> QCoro::Task<void> {
			co_await tile_image_provider::get()->load_image("improvement/" + this->get_identifier() + "/" + terrain->get_identifier() + "/0");
		});
	}

	this->calculate_level();

	named_data_entry::initialize();
}

void improvement::check() const
{
	if (this->get_slot() == improvement_slot::none) {
		throw std::runtime_error(std::format("Improvement \"{}\" has no slot.", this->get_identifier()));
	}

	if (!this->get_resources().empty() && this->get_slot() != improvement_slot::resource) {
		throw std::runtime_error(std::format("Improvement \"{}\" has resources, but is not a resource improvement.", this->get_identifier()));
	}

	if (this->is_visitable() && this->get_slot() != improvement_slot::main) {
		throw std::runtime_error(std::format("Improvement \"{}\" is visitable, but is not a main improvement.", this->get_identifier()));
	}

	if ((this->get_slot() == improvement_slot::main || this->get_slot() == improvement_slot::resource) && this->get_image_filepath().empty()) {
		throw std::runtime_error(std::format("Main or resource improvement \"{}\" has no image filepath.", this->get_identifier()));
	}

	for (const auto &[terrain, filepath] : this->terrain_image_filepaths) {
		bool found = vector::contains(this->get_terrain_types(), terrain);
		if (!found) {
			for (const resource *resource : this->get_resources()) {
				if (vector::contains(resource->get_terrain_types(), terrain)) {
					found = true;
					break;
				}
			}
		}

		if (!found) {
			throw std::runtime_error(std::format("Improvement \"{}\" has a tile image variation for terrain \"{}\", but does not have that terrain in its terrain list, nor do any of its resources have it.", this->get_identifier(), terrain->get_identifier()));
		}
	}

	if (this->get_slot() == improvement_slot::resource && this->icon == nullptr) {
		throw std::runtime_error(std::format("Non-main improvement \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_default_population_class() != nullptr) {
		assert_throw(vector::contains(this->get_population_classes(), this->get_default_population_class()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_free_on_start_conditions() != nullptr) {
		this->get_free_on_start_conditions()->check_validity();
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

bool improvement::can_have_population_type(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	return vector::contains(this->get_population_classes(), population_type->get_population_class());
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
		default:
			break;
	}

	if (!this->get_resources().empty() && !vector::contains(this->get_resources(), site_game_data->get_resource())) {
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

		if (this->get_level() < current_improvement->get_level()) {
			return false;
		}

		if (this->get_level() == current_improvement->get_level()) {
			//the improvement must be better in some way
			return false;
		}
	}

	if (this->get_conditions() != nullptr && !this->get_conditions()->check(site)) {
		return false;
	}

	return true;
}

}
