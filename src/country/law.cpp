#include "metternich.h"

#include "country/law.h"

#include "country/law_group.h"
#include "economy/commodity.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/map_util.h"

namespace metternich {

law::law(const std::string &identifier) : named_data_entry(identifier)
{
}

law::~law()
{
}

void law::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void law::initialize()
{
	if (this->group != nullptr) {
		this->group->add_law(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_law(this);
	}

	named_data_entry::initialize();
}

void law::check() const
{
	if (this->get_group() == nullptr) {
		throw std::runtime_error(std::format("Law \"{}\" has no law group.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Law \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Law \"{}\" has no modifier.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

QVariantList law::get_commodity_costs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_costs());
}

QString law::get_modifier_string(const metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

}
