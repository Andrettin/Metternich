#include "metternich.h"

#include "domain/idea.h"

#include "domain/country_technology.h"
#include "domain/domain.h"
#include "domain/idea_slot.h"
#include "domain/idea_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

idea::idea(const std::string &identifier) : named_data_entry(identifier)
{
}

idea::~idea() = default;

void idea::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<domain>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void idea::check() const
{
	if (this->is_available() && this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Idea \"{}\" of type \"{}\" is available, but has no portrait.", this->get_identifier(), magic_enum::enum_name(this->get_idea_type())));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

bool idea::is_available_for_country_slot(const domain *domain, const idea_slot *slot) const
{
	Q_UNUSED(slot);

	assert_throw(this->get_idea_type() == slot->get_idea_type());

	if (!this->is_available()) {
		return false;
	}

	const country_technology *country_technology = domain->get_technology();

	if (this->get_required_technology() != nullptr && !country_technology->has_technology(this->get_required_technology())) {
		return false;
	}

	if (this->get_obsolescence_technology() != nullptr && country_technology->has_technology(this->get_obsolescence_technology())) {
		return false;
	}

	if (this->get_conditions() != nullptr && !this->get_conditions()->check(domain, read_only_context(domain))) {
		return false;
	}

	return true;
}

std::string idea::get_modifier_string(const domain *domain) const
{
	Q_UNUSED(domain);

	std::string str;
	return str;
}

void idea::apply_modifier(const domain *domain, const int multiplier) const
{
	Q_UNUSED(multiplier);

	assert_throw(domain != nullptr);
}

}
