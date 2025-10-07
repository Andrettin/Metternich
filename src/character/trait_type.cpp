#include "metternich.h"

#include "character/trait_type.h"

#include "script/condition/and_condition.h"
#include "script/modifier.h"

namespace metternich {

trait_type::trait_type(const std::string &identifier)
	: named_data_entry(identifier)
{
}

trait_type::~trait_type()
{
}

void trait_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "gain_conditions") {
		auto conditions = std::make_unique<and_condition<character>>();
		conditions->process_gsml_data(scope);
		this->gain_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trait_type::check() const
{
	if (this->get_traits().empty()) {
		throw std::runtime_error(std::format("Trait type \"{}\" has no traits that belong to it.", this->get_identifier()));
	}
}

}
