#include "metternich.h"

#include "unit/civilian_unit_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/resource.h"
#include "infrastructure/pathway.h"
#include "technology/technology.h"
#include "unit/civilian_unit_class.h"
#include "util/assert_util.h"

namespace metternich {

void civilian_unit_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "improvable_resources") {
		for (const std::string &value : values) {
			this->improvable_resources.insert(resource::get(value));
		}
	} else if (tag == "buildable_pathways") {
		for (const std::string &value : values) {
			this->buildable_pathways.insert(pathway::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void civilian_unit_type::initialize()
{
	assert_throw(this->unit_class != nullptr);

	this->unit_class->add_unit_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_civilian_class_unit_type(this->get_unit_class()) == nullptr);

		this->culture->set_civilian_class_unit_type(this->get_unit_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_civilian_class_unit_type(this->get_unit_class()) == nullptr);

		this->cultural_group->set_civilian_class_unit_type(this->get_unit_class(), this);
	} else {
		this->unit_class->set_default_unit_type(this);
	}

	if (this->required_technology != nullptr) {
		assert_throw(this->get_unit_class() != nullptr);
		this->required_technology->add_enabled_civilian_unit(this);
	}

	named_data_entry::initialize();
}

void civilian_unit_type::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
