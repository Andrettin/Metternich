#include "metternich.h"

#include "economy/resource.h"

#include "economy/commodity.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {
	
resource::resource(const std::string &identifier) : named_data_entry(identifier)
{
}

resource::~resource()
{
}

void resource::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "site_types") {
		for (const std::string &value : values) {
			this->site_types.insert(magic_enum::enum_cast<site_type>(value).value());
		}
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		database::process_gsml_data(this->modifier, scope);
	} else if (tag == "improved_modifier") {
		this->improved_modifier = std::make_unique<metternich::modifier<const site>>();
		database::process_gsml_data(this->improved_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void resource::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_resource(this);
	}

	named_data_entry::initialize();
}

void resource::check() const
{
	assert_throw(this->get_commodity() != nullptr);
	assert_throw(this->get_icon() != nullptr);

	if (this->get_tiny_icon() == nullptr) {
		throw std::runtime_error(std::format("Resource \"{}\" has no tiny icon.", this->get_identifier()));
	}

	if (this->get_commodity() != nullptr && this->get_improvements().empty()) {
		throw std::runtime_error(std::format("Resource \"{}\" has a commodity, but no improvements to produce it.", this->get_identifier()));
	}

	if (this->get_site_types().empty()) {
		throw std::runtime_error(std::format("Resource \"{}\" has no site types.", this->get_identifier()));
	}
}

const metternich::icon *resource::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	}

	return this->get_commodity()->get_icon();
}

const terrain_type *resource::get_fallback_terrain(const terrain_type *terrain) const
{
	const std::vector<const terrain_type *> &resource_terrains = this->get_terrain_types();

	for (const terrain_type *fallback_terrain : terrain->get_fallback_terrains()) {
		if (vector::contains(resource_terrains, fallback_terrain)) {
			return fallback_terrain;
		}
	}

	return nullptr;
}

}
