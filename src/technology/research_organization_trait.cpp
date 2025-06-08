#include "metternich.h"

#include "technology/research_organization_trait.h"

#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

research_organization_trait::research_organization_trait(const std::string &identifier)
	: trait_base(identifier)
{
}

research_organization_trait::~research_organization_trait()
{
}

void research_organization_trait::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else if (tag == "scaled_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->scaled_modifier = std::move(modifier);
	} else {
		trait_base::process_gsml_scope(scope);
	}
}

void research_organization_trait::check() const
{
	if (this->get_modifier() == nullptr && this->get_scaled_modifier() == nullptr) {
		throw std::runtime_error(std::format("Research organization trait \"{}\" has no modifier or scaled modifier.", this->get_identifier()));
	}

	trait_base::check();
}

}
