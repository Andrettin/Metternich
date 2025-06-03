#include "metternich.h"

#include "country/office.h"

#include "character/character_attribute.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"

namespace metternich {

office::office(const std::string &identifier) : named_data_entry(identifier)
{
}

office::~office()
{
}

void office::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "holder_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		database::process_gsml_data(conditions, scope);
		this->holder_conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void office::check() const
{
	assert_throw(this->get_attribute() != character_attribute::none);
}

}
