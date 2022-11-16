#include "metternich.h"

#include "country/culture.h"

#include "country/cultural_group.h"
#include "infrastructure/building_class.h"
#include "population/population_class.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"

namespace metternich {

void culture::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Culture \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	data_entry::initialize();
}

void culture::check() const
{
	assert_throw(this->get_group() != nullptr);
	assert_throw(this->get_color().isValid());

	if (this->get_default_phenotype() == nullptr) {
		throw std::runtime_error("Culture \"" + this->get_identifier() + "\" has no default phenotype.");
	}

	culture_base::check();
}

const phenotype *culture::get_default_phenotype() const
{
	if (this->default_phenotype != nullptr) {
		return this->default_phenotype;
	}

	return this->get_group()->get_default_phenotype();
}

const building_type *culture::get_building_class_type(const building_class *building_class) const
{
	const building_type *type = culture_base::get_building_class_type(building_class);
	if (type != nullptr) {
		return type;
	}

	if (this->get_group() != nullptr) {
		type = this->get_group()->get_building_class_type(building_class);
		if (type != nullptr) {
			return type;
		}
	}

	return building_class->get_default_building_type();

}

const population_type *culture::get_population_class_type(const population_class *population_class) const
{
	const population_type *type = culture_base::get_population_class_type(population_class);
	if (type != nullptr) {
		return type;
	}

	if (this->get_group() != nullptr) {
		type = this->get_group()->get_population_class_type(population_class);
		if (type != nullptr) {
			return type;
		}
	}

	return population_class->get_default_population_type();

}

}
