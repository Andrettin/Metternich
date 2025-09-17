#include "metternich.h"

#include "domain/idea_slot.h"

#include "script/condition/and_condition.h"

namespace metternich {

idea_slot::idea_slot(const std::string &identifier)
	: named_data_entry(identifier)
{
}

idea_slot::~idea_slot() = default;

void idea_slot::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

}
