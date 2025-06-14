#include "metternich.h"

#include "technology/research_organization_slot.h"

#include "script/condition/and_condition.h"

namespace metternich {

research_organization_slot::research_organization_slot(const std::string &identifier)
	: named_data_entry(identifier)
{
}

research_organization_slot::~research_organization_slot() = default;

void research_organization_slot::process_gsml_scope(const gsml_data &scope)
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

}
