#include "metternich.h"

#include "species/taxon_base.h"

#include "character/starting_age_category.h"
#include "database/gsml_data.h"
#include "item/item_slot.h"
#include "language/fallback_name_generator.h"
#include "language/gendered_name_generator.h"
#include "language/name_generator.h"
#include "species/taxon.h"
#include "util/gender.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

taxon_base::taxon_base(const std::string &identifier) : named_data_entry(identifier)
{
}

taxon_base::~taxon_base()
{
}

void taxon_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "starting_age_modifiers") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->starting_age_modifiers[magic_enum::enum_cast<starting_age_category>(key).value()] = dice(value);
		});
	} else if (tag == "item_slots") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->item_slot_counts[item_slot::get(key)] = std::stoi(value);
		});
	} else if (tag == "given_names") {
		if (this->given_name_generator == nullptr) {
			this->given_name_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->given_name_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<archimedes::gender>::to_enum(tag);

			this->given_name_generator->add_names(gender, child_scope.get_values());
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void taxon_base::initialize()
{
	if (this->get_supertaxon() != nullptr) {
		if (!this->get_supertaxon()->is_initialized()) {
			this->get_supertaxon()->initialize();
		}

		this->get_supertaxon()->add_given_names_from(this);
	}

	if (this->given_name_generator != nullptr) {
		fallback_name_generator::get()->add_given_names(this->given_name_generator);
		this->given_name_generator->propagate_ungendered_names();
	}

	data_entry::initialize();
}

const taxon *taxon_base::get_supertaxon_of_rank(const taxonomic_rank rank) const
{
	if (this->get_supertaxon() == nullptr) {
		return nullptr;
	}

	if (this->get_supertaxon()->get_rank() > rank) {
		return nullptr;
	}

	if (this->get_supertaxon()->get_rank() == rank) {
		return this->get_supertaxon();
	}

	return this->get_supertaxon()->get_supertaxon_of_rank(rank);
}

bool taxon_base::is_subtaxon_of(const taxon *other_taxon) const
{
	if (this->get_supertaxon() == nullptr) {
		return false;
	}

	if (other_taxon->get_rank() <= this->get_rank()) {
		return false;
	}

	if (other_taxon == this->get_supertaxon()) {
		return true;
	}

	return this->get_supertaxon()->is_subtaxon_of(other_taxon);
}

bool taxon_base::is_ethereal() const
{
	if (this->ethereal) {
		return true;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->is_ethereal();
	}

	return false;
}

int taxon_base::get_adulthood_age() const
{
	if (this->adulthood_age != 0) {
		return this->adulthood_age;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_adulthood_age();
	}

	return this->adulthood_age;
}

int taxon_base::get_middle_age() const
{
	if (this->middle_age != 0) {
		return this->middle_age;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_middle_age();
	}

	return this->middle_age;
}

int taxon_base::get_old_age() const
{
	if (this->old_age != 0) {
		return this->old_age;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_old_age();
	}

	return this->old_age;
}

int taxon_base::get_venerable_age() const
{
	if (this->venerable_age != 0) {
		return this->venerable_age;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_venerable_age();
	}

	return this->venerable_age;
}

const dice &taxon_base::get_maximum_age_modifier() const
{
	if (!this->maximum_age_modifier.is_null()) {
		return this->maximum_age_modifier;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_maximum_age_modifier();
	}

	return this->maximum_age_modifier;
}

const dice &taxon_base::get_starting_age_modifier(const starting_age_category category) const
{
	const auto find_iterator = this->starting_age_modifiers.find(category);

	if (find_iterator != this->starting_age_modifiers.end()) {
		return find_iterator->second;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_starting_age_modifier(category);
	}

	static constexpr dice dice;
	return dice;
}

const data_entry_map<item_slot, int> &taxon_base::get_item_slot_counts() const
{
	return this->item_slot_counts;
}

int taxon_base::get_item_slot_count(const item_slot *slot) const
{
	const auto find_iterator = this->item_slot_counts.find(slot);

	if (find_iterator != this->item_slot_counts.end()) {
		return find_iterator->second;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_item_slot_count(slot);
	}

	return 0;
}

const name_generator *taxon_base::get_given_name_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->given_name_generator != nullptr) {
		name_generator = this->given_name_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr && name_generator->has_enough_data()) {
		return name_generator;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_given_name_generator(gender);
	}

	return name_generator;
}

void taxon_base::add_given_name(const gender gender, const name_variant &name)
{
	if (this->given_name_generator == nullptr) {
		this->given_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->given_name_generator->add_name(gender, name);

	if (gender == gender::none) {
		this->given_name_generator->add_name(gender::male, name);
		this->given_name_generator->add_name(gender::female, name);
	}

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_given_name(gender, name);
	}
}

void taxon_base::add_given_names_from(const taxon_base *other)
{
	if (other->given_name_generator != nullptr) {
		if (this->given_name_generator == nullptr) {
			this->given_name_generator = std::make_unique<gendered_name_generator>();
		}

		this->given_name_generator->add_names_from(other->given_name_generator);
	}

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_given_names_from(other);
	}
}

}
