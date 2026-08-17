#include "metternich.h"

#include "domain/domain_attribute.h"

#include "script/modifier.h"

namespace metternich {

domain_attribute::domain_attribute(const std::string &identifier) : named_data_entry(identifier)
{
}

domain_attribute::~domain_attribute()
{
}

void domain_attribute::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "value_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value = std::stoi(child_tag);
			if (!this->value_modifiers.contains(value)) {
				this->value_modifiers[value] = std::make_unique<metternich::modifier<const domain>>();
			}
			this->value_modifiers[value]->process_gsml_data(child_scope);
		});
	} else if (tag == "recurring_value_modifiers") {
		static constexpr int max_value = std::numeric_limits<uint8_t>::max();

		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value_interval = std::stoi(child_tag);
			for (int i = value_interval; i <= max_value; i += value_interval) {
				if (!this->value_modifiers.contains(i)) {
					this->value_modifiers[i] = std::make_unique<metternich::modifier<const domain>>();
				}
				this->value_modifiers[i]->process_gsml_data(child_scope);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void domain_attribute::check() const
{
	if (this->get_check_dice().is_null()) {
		throw std::runtime_error(std::format("Domain attribute \"{}\" has no check dice.", this->get_identifier()));
	}

	if (this->get_check_dice().get_count() != 1) {
		throw std::runtime_error(std::format("Domain attribute \"{}\" has check dice with a dice count different than 1.", this->get_identifier()));
	}

	named_data_entry::check();
}

}
