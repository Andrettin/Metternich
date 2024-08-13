#include "metternich.h"

#include "unit/military_unit_class.h"

#include "language/name_generator.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

void military_unit_class::propagate_names(const military_unit_class_map<std::unique_ptr<name_generator>> &name_generators, std::unique_ptr<name_generator> &ship_name_generator)
{
	for (const auto &kv_pair : name_generators) {
		const military_unit_class *unit_class = kv_pair.first;

		if (unit_class->is_ship()) {
			if (ship_name_generator == nullptr) {
				ship_name_generator = std::make_unique<name_generator>();
			}

			ship_name_generator->add_names(kv_pair.second->get_names());
		}
	}
}

military_unit_class::military_unit_class(const std::string &identifier)
	: named_data_entry(identifier), domain(military_unit_domain::none), category(military_unit_category::none)
{
}

void military_unit_class::check() const
{
	assert_throw(this->get_domain() != military_unit_domain::none);
	assert_throw(this->get_category() != military_unit_category::none);
}

bool military_unit_class::is_animal() const
{
	switch (this->get_category()) {
		case military_unit_category::beasts:
		case military_unit_category::colossal_beasts:
		case military_unit_category::sea_beasts:
		case military_unit_category::colossal_sea_beasts:
		case military_unit_category::flying_beasts:
		case military_unit_category::colossal_flying_beasts:
			return true;
		default:
			return false;
	}
}

bool military_unit_class::is_ship() const
{
	return is_ship_military_unit_category(this->get_category());
}

bool military_unit_class::is_leader() const
{
	return is_leader_military_unit_category(this->get_category());
}

void military_unit_class::set_default_unit_type(const military_unit_type *unit_type)
{
	if (this->get_default_unit_type() != nullptr) {
		throw std::runtime_error(std::format("Cannot set \"{}\" as the default military unit type of class \"{}\", as it already has \"{}\" as its default type.", unit_type->get_identifier(), this->get_identifier(), this->get_default_unit_type()->get_identifier()));
	}

	this->default_unit_type = unit_type;
}

}
