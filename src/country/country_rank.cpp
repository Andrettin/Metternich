#include "metternich.h"

#include "country/country_rank.h"

#include "script/condition/and_condition.h"

namespace metternich {

country_rank::country_rank(const std::string &identifier) : named_data_entry(identifier)
{
}

country_rank::~country_rank()
{
}

void country_rank::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void country_rank::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

}
