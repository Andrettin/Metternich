#include "metternich.h"

#include "country/law.h"

#include "country/law_group.h"
#include "script/modifier.h"
#include "technology/technology.h"

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

	if (tag == "modifier") {
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
}

QString law::get_modifier_string(const metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

}
