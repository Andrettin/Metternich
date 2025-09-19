#include "metternich.h"

#include "domain/idea.h"

#include "domain/country_technology.h"
#include "domain/domain.h"
#include "domain/idea_trait.h"
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

	if (this->get_traits().empty()) {
		//throw std::runtime_error(std::format("Idea \"{}\" of type \"{}\" has no traits.", this->get_identifier(), magic_enum::enum_name(this->get_idea_type())));
	}

	if (this->get_skill() == 0) {
		for (const idea_trait *trait : this->get_traits()) {
			if (trait->get_scaled_modifier() != nullptr) {
				throw std::runtime_error(std::format("Idea \"{}\" of type \"{}\" has a trait with a scaled modifier, but no skill value.", this->get_identifier(), magic_enum::enum_name(this->get_idea_type())));
			}
		}
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

	for (const idea_trait *trait : this->get_traits()) {
		if (trait->get_conditions() != nullptr && !trait->get_conditions()->check(domain, read_only_context(domain))) {
			return false;
		}
	}

	return true;
}

std::string idea::get_modifier_string(const domain *domain) const
{
	std::string str;

	for (const idea_trait *trait : this->get_traits()) {
		if (trait->get_modifier() == nullptr && trait->get_scaled_modifier() == nullptr) {
			continue;
		}

		if (!str.empty()) {
			str += "\n";
		}

		if (!trait->has_hidden_name()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += string::highlight(trait->get_name());
		}

		const size_t indent = trait->has_hidden_name() ? 0 : 1;

		if (trait->get_modifier() != nullptr) {
			str += "\n" + trait->get_modifier()->get_string(domain, 1, indent);
		}

		if (trait->get_scaled_modifier() != nullptr) {
			str += "\n" + trait->get_scaled_modifier()->get_string(domain, std::min(this->get_skill(), trait->get_max_scaling()), indent);
		}
	}

	return str;
}

void idea::apply_modifier(const domain *domain, const int multiplier) const
{
	assert_throw(domain != nullptr);

	for (const idea_trait *trait : this->get_traits()) {
		this->apply_trait_modifier(trait, domain, multiplier);
	}
}

void idea::apply_trait_modifier(const idea_trait *trait, const domain *domain, const int multiplier) const
{
	if (trait->get_modifier() != nullptr) {
		trait->get_modifier()->apply(domain, multiplier);
	}

	if (trait->get_scaled_modifier() != nullptr) {
		trait->get_scaled_modifier()->apply(domain, std::min(this->get_skill(), trait->get_max_scaling()) * multiplier);
	}
}

}
