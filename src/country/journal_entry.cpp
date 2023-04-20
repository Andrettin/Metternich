#include "metternich.h"

#include "country/journal_entry.h"

#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"

namespace metternich {

journal_entry::journal_entry(const std::string &identifier) : named_data_entry(identifier)
{
}

journal_entry::~journal_entry()
{
}

void journal_entry::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "preconditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->preconditions = std::move(conditions);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "completion_conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->completion_conditions = std::move(conditions);
	} else if (tag == "failure_conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->failure_conditions = std::move(conditions);
	} else if (tag == "completion_effects") {
		auto effects = std::make_unique<effect_list<const country>>();
		database::process_gsml_data(effects, scope);
		this->completion_effects = std::move(effects);
	} else if (tag == "failure_effects") {
		auto effects = std::make_unique<effect_list<const country>>();
		database::process_gsml_data(effects, scope);
		this->failure_effects = std::move(effects);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void journal_entry::check() const
{
	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Journal entry \"{}\" does not have a portrait.", this->get_identifier()));
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_completion_conditions() != nullptr) {
		this->get_completion_conditions()->check_validity();
	}

	if (this->get_failure_conditions() != nullptr) {
		this->get_failure_conditions()->check_validity();
	}

	if (this->get_completion_effects() != nullptr) {
		this->get_completion_effects()->check();
	}

	if (this->get_failure_effects() != nullptr) {
		this->get_failure_effects()->check();
	}
}

}
