#include "politics/government_type.h"

#include "holding/holding_type.h"
#include "script/condition/and_condition.h"
#include "util/container_util.h"

namespace metternich {

government_type::government_type(const std::string &identifier) : data_entry(identifier)
{
}

government_type::~government_type()
{
}

void government_type::process_gsml_scope(const gsml_data &scope)
{
	if (scope.get_tag() == "conditions") {
		this->conditions = std::make_unique<and_condition<character>>();
		database::process_gsml_data(this->conditions.get(), scope);
	} else {
		data_entry_base::process_gsml_scope(scope);
	}
}

QVariantList government_type::get_allowed_holding_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_allowed_holding_types());
}

}
