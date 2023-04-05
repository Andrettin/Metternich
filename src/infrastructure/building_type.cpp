#include "metternich.h"

#include "infrastructure/building_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/production_type.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

building_type::building_type(const std::string &identifier) : named_data_entry(identifier)
{
}

building_type::~building_type()
{
}

void building_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "production_types") {
		for (const std::string &value : values) {
			this->production_types.push_back(production_type::get(value));
		}
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void building_type::initialize()
{
	assert_throw(this->building_class != nullptr);
	this->building_class->add_building_type(this);
	this->building_class->get_slot_type()->add_building_type(this);

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_building_class_type(this->get_building_class()) == nullptr);

		this->culture->set_building_class_type(this->get_building_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_building_class_type(this->get_building_class()) == nullptr);

		this->cultural_group->set_building_class_type(this->get_building_class(), this);
	} else {
		this->building_class->set_default_building_type(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_building(this);
	}

	data_entry::initialize();
}

void building_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
	assert_throw(this->get_icon() != nullptr);

	if (!this->get_production_types().empty()) {
		assert_throw(this->get_base_capacity() > 0);
	}
}

QVariantList building_type::get_production_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_production_types());
}

QString building_type::get_country_modifier_string() const
{
	if (this->get_country_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_country_modifier()->get_string());
}

}
