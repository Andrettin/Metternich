#include "metternich.h"

#include "map/terrain_type.h"

#include "database/database.h"
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
