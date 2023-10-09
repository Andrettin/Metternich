#include "metternich.h"

#include "country/country_tier_data.h"

#include "country/country_tier.h"
#include "script/modifier.h"

namespace metternich {

country_tier_data::country_tier_data(const std::string &identifier) : named_data_entry(identifier)
{
}

country_tier_data::~country_tier_data()
{
}

void country_tier_data::process_gsml_scope(const gsml_data &scope)
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

void country_tier_data::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Country tier \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Country tier \"{}\" does not have a modifier.", this->get_identifier()));
	}
}

QString country_tier_data::get_modifier_string(metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

}
