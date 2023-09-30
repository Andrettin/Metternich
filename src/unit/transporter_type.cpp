#include "metternich.h"

#include "unit/transporter_type.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "technology/technology.h"
#include "unit/transporter_category.h"
#include "unit/transporter_class.h"
#include "util/assert_util.h"

namespace metternich {

void transporter_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void transporter_type::initialize()
{
	assert_throw(this->transporter_class != nullptr);

	this->transporter_class->add_transporter_type(this);

	if (this->culture != nullptr) {
		this->culture->set_transporter_class_type(this->get_transporter_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_transporter_class_type(this->get_transporter_class()) == nullptr);

		this->cultural_group->set_transporter_class_type(this->get_transporter_class(), this);
	} else {
		this->transporter_class->set_default_transporter_type(this);
	}

	if (this->required_technology != nullptr) {
		assert_throw(this->get_transporter_class() != nullptr);
		this->required_technology->add_enabled_transporter(this);
	}

	named_data_entry::initialize();
}

void transporter_type::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

transporter_category transporter_type::get_category() const
{
	if (this->get_transporter_class() == nullptr) {
		return transporter_category::none;
	}

	return this->get_transporter_class()->get_category();
}

bool transporter_type::is_ship() const
{
	switch (this->get_category()) {
		case transporter_category::small_merchant_ship:
		case transporter_category::large_merchant_ship:
			return true;
		default:
			return false;
	}
}

}
