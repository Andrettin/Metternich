#include "metternich.h"

#include "character/trait_type.h"

#include "character/trait.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/vector_util.h"

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
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "upper_types") {
		for (const std::string &value : values) {
			this->upper_types.push_back(trait_type::get(value));
		}
	} else if (tag == "gain_conditions") {
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

void trait_type::initialize()
{
	for (trait_type *upper_type : this->upper_types) {
		for (trait *trait : this->get_traits()) {
			if (!vector::contains(upper_type->get_traits(), trait)) {
				upper_type->add_trait(trait);
				trait->add_type(upper_type);
			}
		}
	}

	named_data_entry::initialize();
}

void trait_type::check() const
{
	if (this->get_traits().empty()) {
		throw std::runtime_error(std::format("Trait type \"{}\" has no traits that belong to it.", this->get_identifier()));
	}
}

void trait_type::add_trait(trait *trait)
{
	this->traits.push_back(trait);

	for (trait_type *upper_type : this->upper_types) {
		if (!vector::contains(upper_type->get_traits(), trait)) {
			upper_type->add_trait(trait);
			trait->add_type(upper_type);
		}
	}
}

}
