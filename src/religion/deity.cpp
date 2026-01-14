#include "metternich.h"

#include "religion/deity.h"

#include "character/character.h"
#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "domain/idea_type.h"
#include "religion/deity_slot.h"
#include "religion/divine_domain.h"
#include "religion/religion.h"
#include "technology/technology.h"
#include "util/vector_util.h"

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

deity::deity(const std::string &identifier) : idea(identifier)
{
}

deity::~deity() = default;

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
	} else if (tag == "major_domains") {
		for (const std::string &value : values) {
			const divine_domain *domain = divine_domain::get(value);
			this->major_domains.push_back(domain);
		}
	} else if (tag == "minor_domains") {
		for (const std::string &value : values) {
			const divine_domain *domain = divine_domain::get(value);
			this->minor_domains.push_back(domain);
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
		this->character->process_gsml_data(scope);
	} else {
		idea::process_gsml_scope(scope);
	}
}

void deity::initialize()
{
	if (this->get_required_technology() != nullptr) {
		this->get_required_technology()->add_enabled_deity(this);
	}

	if (this->get_obsolescence_technology() != nullptr) {
		this->get_obsolescence_technology()->add_disabled_deity(this);
	}

	idea::initialize();
}

void deity::check() const
{
	if (this->get_pantheon() == nullptr) {
		throw std::runtime_error(std::format("Deity \"{}\" has no pantheon.", this->get_identifier()));
	}

	if (this->get_religions().empty()) {
		throw std::runtime_error(std::format("Deity \"{}\" is not worshipped by any religions.", this->get_identifier()));
	}

	if (this->get_major_domains().empty()) {
		throw std::runtime_error(std::format("Deity \"{}\" has no major domains.", this->get_identifier()));
	}


	idea::check();
}

idea_type deity::get_idea_type() const
{
	return idea_type::deity;
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

bool deity::is_available_for_country_slot(const domain *domain, const idea_slot *slot) const
{
	if (!idea::is_available_for_country_slot(domain, slot)) {
		return false;
	}

	const deity_slot *deity_slot = static_cast<const metternich::deity_slot *>(slot);

	if (this->is_major() != deity_slot->is_major()) {
		return false;
	}

	const domain_game_data *domain_game_data = domain->get_game_data();

	if (!vector::contains(this->get_religions(), domain_game_data->get_religion())) {
		return false;
	}

	return true;
}

}
