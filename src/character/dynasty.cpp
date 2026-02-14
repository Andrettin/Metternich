#include "metternich.h"

#include "character/dynasty.h"

#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "util/gender.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

void dynasty::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "cultural_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->cultural_names[culture::get(key)][gender::none] = value;
		});

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const metternich::culture *culture = culture::get(child_tag);

			scope.for_each_property([&](const gsml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();

				this->cultural_names[culture][magic_enum::enum_cast<gender>(key).value()] = value;
			});
		});
	} else if (tag == "cultural_group_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->cultural_names[cultural_group::get(key)][gender::none] = value;
		});

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const metternich::cultural_group *cultural_group = cultural_group::get(child_tag);

			scope.for_each_property([&](const gsml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();

				this->cultural_names[cultural_group][magic_enum::enum_cast<gender>(key).value()] = value;
			});
		});
	} else if (tag == "gendered_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->cultural_names[nullptr][magic_enum::enum_cast<gender>(key).value()] = value;
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

std::string dynasty::get_surname(const metternich::culture *culture, const gender gender) const
{
	std::string surname;

	if (!this->get_prefix().empty()) {
		surname = this->get_prefix();

		if (!this->contracted_prefix) {
			surname += " ";
		}
	}

	auto culture_find_it = this->cultural_names.find(culture);
	const cultural_group *cultural_group = culture->get_group();
	while (culture_find_it == this->cultural_names.end() && cultural_group != nullptr) {
		culture_find_it = this->cultural_names.find(cultural_group);
		cultural_group = cultural_group->get_group();
	}
	if (culture_find_it == this->cultural_names.end()) {
		culture_find_it = this->cultural_names.find(nullptr);
	}
	if (culture_find_it != this->cultural_names.end()) {
		auto gender_find_it = culture_find_it->second.find(gender);
		if (gender_find_it == culture_find_it->second.end()) {
			gender_find_it = culture_find_it->second.find(gender::none);
		}
		if (gender_find_it != culture_find_it->second.end()) {
			surname += gender_find_it->second;
			return surname;
		}
	}

	surname += this->get_name();
	return surname;
}

}
