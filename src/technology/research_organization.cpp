#include "metternich.h"

#include "technology/research_organization.h"

#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "technology/research_organization_trait.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

research_organization::research_organization(const std::string &identifier) : named_data_entry(identifier)
{
}

research_organization::~research_organization() = default;

void research_organization::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "traits") {
		for (const std::string &value : values) {
			this->traits.push_back(research_organization_trait::get(value));
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void research_organization::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_research_organization(this);
	}

	if (this->obsolescence_technology != nullptr) {
		this->obsolescence_technology->add_disabled_research_organization(this);
	}

	named_data_entry::initialize();
}

void research_organization::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_traits().empty()) {
		throw std::runtime_error(std::format("Research organization \"{}\" has no traits.", this->get_identifier()));
	}

	if (this->get_skill() == 0) {
		for (const research_organization_trait *trait : this->get_traits()) {
			if (trait->get_scaled_modifier() != nullptr) {
				throw std::runtime_error(std::format("Research organization \"{}\" has a trait with a scaled modifier, but no skill value.", this->get_identifier()));
			}
		}
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

std::string research_organization::get_modifier_string(const country *country) const
{
	std::string str;

	for (const research_organization_trait *trait : this->get_traits()) {
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
			str += "\n" + trait->get_modifier()->get_string(country, 1, indent);
		}

		if (trait->get_scaled_modifier() != nullptr) {
			str += "\n" + trait->get_scaled_modifier()->get_string(country, std::min(this->get_skill(), trait->get_max_scaling()), indent);
		}
	}

	return str;
}

void research_organization::apply_modifier(const country *country, const int multiplier) const
{
	assert_throw(country != nullptr);

	for (const research_organization_trait *trait : this->get_traits()) {
		this->apply_trait_modifier(trait, country, multiplier);
	}
}

void research_organization::apply_trait_modifier(const research_organization_trait *trait, const country *country, const int multiplier) const
{
	if (trait->get_modifier() != nullptr) {
		trait->get_modifier()->apply(country, multiplier);
	}

	if (trait->get_scaled_modifier() != nullptr) {
		trait->get_scaled_modifier()->apply(country, std::min(this->get_skill(), trait->get_max_scaling()) * multiplier);
	}
}

}
