#include "metternich.h"

#include "country/idea_trait.h"

#include "script/condition/and_condition.h"
#include "script/modifier.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

idea_trait::idea_trait(const std::string &identifier)
	: trait_base(identifier)
{
}

idea_trait::~idea_trait()
{
}

void idea_trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else if (tag == "scaled_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		modifier->process_gsml_data(scope);
		this->scaled_modifier = std::move(modifier);
	} else {
		trait_base::process_gsml_scope(scope);
	}
}

void idea_trait::check() const
{
	if (this->get_modifier() == nullptr && this->get_scaled_modifier() == nullptr) {
		throw std::runtime_error(std::format("Idea trait \"{}\" for type \"{}\" has no modifier or scaled modifier.", this->get_identifier(), magic_enum::enum_name(this->get_idea_type())));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	trait_base::check();
}

}
