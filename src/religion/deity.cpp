#include "metternich.h"

#include "religion/deity.h"

#include "character/character.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "database/defines.h"
#include "map/province.h"
#include "religion/religion.h"
#include "util/string_util.h"

namespace metternich {

deity *deity::add(const std::string &identifier, const metternich::data_module *data_module)
{
	deity *deity = data_type::add(identifier, data_module);

	//add a character with the same identifier as the deity for it
	metternich::character *character = character::add(identifier, data_module);
	character->set_deity(deity);
	deity->character = character;

	return deity;
}

void deity::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "religions") {
		for (const std::string &value : values) {
			religion *religion = religion::get(value);
			religion->add_deity(this);
			this->religions.push_back(religion);
		}
	} else if (tag == "cultural_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const culture *culture = culture::get(property.get_key());
			this->cultural_names[culture] = property.get_value();
		});
	} else if (tag == "cultural_group_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const cultural_group *cultural_group = cultural_group::get(property.get_key());
			this->cultural_group_names[cultural_group] = property.get_value();
		});
	} else if (tag == "character") {
		database::process_gsml_data(this->character, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void deity::check() const
{
	if (this->get_pantheon() == nullptr) {
		throw std::runtime_error(std::format("Deity \"{}\" has no pantheon.", this->get_identifier()));
	}

	if (this->get_religions().empty()) {
		throw std::runtime_error(std::format("Deity \"{}\" is not worshipped by any religions.", this->get_identifier()));
	}
}

const std::string &deity::get_cultural_name(const culture *culture) const
{
	if (culture != nullptr) {
		const auto find_iterator = this->cultural_names.find(culture);
		if (find_iterator != this->cultural_names.end()) {
			return find_iterator->second;
		}

		if (culture->get_group() != nullptr) {
			return this->get_cultural_name(culture->get_group());
		}
	}

	return this->get_name();
}

const std::string &deity::get_cultural_name(const cultural_group *cultural_group) const
{
	if (cultural_group != nullptr) {
		const auto group_find_iterator = this->cultural_group_names.find(cultural_group);
		if (group_find_iterator != this->cultural_group_names.end()) {
			return group_find_iterator->second;
		}

		if (cultural_group->get_upper_group() != nullptr) {
			return this->get_cultural_name(cultural_group->get_upper_group());
		}
	}

	return this->get_name();
}

}
