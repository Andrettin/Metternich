#include "metternich.h"

#include "domain/domain_tier_data.h"

#include "domain/domain_tier.h"
#include "script/modifier.h"

namespace metternich {

domain_tier_data::domain_tier_data(const std::string &identifier) : named_data_entry(identifier)
{
}

domain_tier_data::~domain_tier_data()
{
}

void domain_tier_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const domain>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void domain_tier_data::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Domain tier \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_modifier() == nullptr) {
		//throw std::runtime_error(std::format("Domain tier \"{}\" does not have a modifier.", this->get_identifier()));
	}
}

QString domain_tier_data::get_modifier_string(metternich::domain *domain) const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_single_line_string(domain));
}

}
