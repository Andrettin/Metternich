#include "metternich.h"

#include "economy/resource.h"

#include "economy/commodity.h"
#include "map/terrain_type.h"
#include "technology/technology.h"
#include "util/assert_util.h"

namespace metternich {
	
void resource::process_gsml_scope(const gsml_data &scope)
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
}

const metternich::icon *resource::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	}

	return this->get_commodity()->get_icon();
}

}
