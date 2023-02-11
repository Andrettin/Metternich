#include "metternich.h"

#include "infrastructure/improvement.h"

#include "economy/employment_type.h"
#include "economy/resource.h"
#include "map/terrain_type.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
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
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void improvement::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_improvement(this);
	}

	data_entry::initialize();
}

void improvement::check() const
{
	assert_throw(this->get_resource() != nullptr || !this->get_terrain_types().empty());

	if (this->get_output_commodity() != nullptr) {
		assert_throw(this->get_output_multiplier() > 0);
	}

	if (this->get_employment_type() != nullptr) {
		assert_throw(this->get_resource() != nullptr);
		assert_throw(this->get_employment_type()->get_output_commodity() == this->get_resource()->get_commodity());
		assert_throw(this->get_employment_capacity() > 0);
	}

	assert_log(!this->get_image_filepath().empty());

	for (const auto &[terrain, filepath] : this->terrain_image_filepaths) {
		assert_throw(vector::contains(this->get_terrain_types(), terrain) || vector::contains(this->get_resource()->get_terrain_types(), terrain));
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
	if (this->get_resource() != nullptr) {
		return this->get_resource()->get_commodity();
	}

	return nullptr;
}

}
