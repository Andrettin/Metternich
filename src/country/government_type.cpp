#include "metternich.h"

#include "country/government_type.h"

#include "script/modifier.h"
#include "technology/technology.h"

namespace metternich {

government_type::government_type(const std::string &identifier) : named_data_entry(identifier)
{
}

government_type::~government_type()
{
}

void government_type::process_gsml_scope(const gsml_data &scope)
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

void government_type::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_government_type(this);
	}

	named_data_entry::initialize();
}

void government_type::check() const
{
	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Government type \"{}\" does not have a modifier.", this->get_identifier()));
	}
}

QString government_type::get_modifier_string() const
{
	return QString::fromStdString(this->get_modifier()->get_string());
}

}
