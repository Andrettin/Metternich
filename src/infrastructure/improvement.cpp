#include "metternich.h"

#include "infrastructure/improvement.h"

#include "economy/resource.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"

namespace metternich {
	
void improvement::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void improvement::check() const
{
	assert_throw(this->get_resource() != nullptr || !this->get_terrain_types().empty());

	if (this->get_output_commodity() != nullptr) {
		assert_throw(this->get_output_value() > 0);
	}

	assert_log(!this->get_image_filepath().empty());
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
