#include "metternich.h"

#include "country/office.h"

#include "character/character_attribute.h"
#include "database/defines.h"
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

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "holder_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->holder_conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void office::check() const
{
	assert_throw(this->get_attribute() != nullptr);

	if (this->is_ruler()) {
		assert_throw(!this->is_minister());
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_holder_conditions() != nullptr) {
		this->get_holder_conditions()->check_validity();
	}
}

bool office::is_ruler() const
{
	return defines::get()->get_ruler_office() == this;
}

}
