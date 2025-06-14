#include "metternich.h"

#include "religion/deity_trait.h"

#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

deity_trait::deity_trait(const std::string &identifier)
	: trait_base(identifier)
{
}

deity_trait::~deity_trait()
{
}

void deity_trait::process_gsml_scope(const gsml_data &scope)
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
	} else {
		trait_base::process_gsml_scope(scope);
	}
}

void deity_trait::check() const
{
	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Deity trait \"{}\" has no modifier.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	trait_base::check();
}

}
