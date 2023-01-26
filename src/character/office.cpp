#include "metternich.h"

#include "character/office.h"

#include "character/office_type.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"

namespace metternich {

office::office(const std::string &identifier)
	: named_data_entry(identifier), type(office_type::none)
{
}

office::~office()
{
}

void office::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "country_conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->country_conditions = std::move(conditions);
	} else if (tag == "province_conditions") {
		auto conditions = std::make_unique<and_condition<province>>();
		database::process_gsml_data(conditions, scope);
		this->province_conditions = std::move(conditions);
	} else if (tag == "character_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		database::process_gsml_data(conditions, scope);
		this->character_conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void office::check() const
{
	if (this->get_type() == office_type::none) {
		throw std::runtime_error("Office \"" + this->get_identifier() + "\" has no type.");
	}

	if (this->get_country_conditions() != nullptr) {
		this->get_country_conditions()->check_validity();
	}

	if (this->get_province_conditions() != nullptr) {
		if (this->get_type() != office_type::province) {
			throw std::runtime_error("Office \"" + this->get_identifier() + "\" has province conditions, but is not a provincial office.");
		}

		this->get_province_conditions()->check_validity();
	}

	if (this->get_character_conditions() != nullptr) {
		this->get_character_conditions()->check_validity();
	}
}

}
