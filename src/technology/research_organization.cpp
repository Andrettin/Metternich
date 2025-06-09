#include "metternich.h"

#include "technology/research_organization.h"

#include "script/condition/and_condition.h"
#include "technology/research_organization_trait.h"
#include "technology/technology.h"

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

}
