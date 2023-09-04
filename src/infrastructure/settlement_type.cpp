#include "metternich.h"

#include "infrastructure/settlement_type.h"

#include "script/condition/and_condition.h"
#include "util/vector_util.h"

namespace metternich {

settlement_type::settlement_type(const std::string &identifier) : named_data_entry(identifier)
{
}

settlement_type::~settlement_type()
{
}

void settlement_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "base_settlement_types") {
		for (const std::string &value : values) {
			settlement_type *settlement_type = settlement_type::get(value);
			this->base_settlement_types.push_back(settlement_type);
			settlement_type->upgraded_settlement_types.push_back(this);
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void settlement_type::check() const
{
	if (vector::contains(this->get_base_settlement_types(), this)) {
		throw std::runtime_error(std::format("Settlement type \"{}\" is an upgrade of itself.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

}
