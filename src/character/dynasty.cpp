#include "metternich.h"

#include "character/dynasty.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

void dynasty::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "gendered_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->gendered_names[magic_enum::enum_cast<gender>(key).value()] = value;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void dynasty::check() const
{
	if (this->get_culture() == nullptr) {
		throw std::runtime_error(std::format("Dynasty \"{}\" has no culture.", this->get_identifier()));
	}
}

std::string dynasty::get_surname(const gender gender) const
{
	std::string surname;

	if (!this->get_prefix().empty()) {
		surname = this->get_prefix();

		if (!this->contracted_prefix) {
			surname += " ";
		}
	}

	const auto find_it = this->gendered_names.find(gender);
	if (find_it != this->gendered_names.end()) {
		surname += find_it->second;
	} else {
		surname += this->get_name();
	}

	return surname;
}

}
