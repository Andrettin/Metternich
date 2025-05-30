#include "metternich.h"

#include "species/phenotype.h"

#include "species/species.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/vector_util.h"

namespace metternich {

void phenotype::initialize()
{
	if (!this->color.isValid()) {
		log::log_error(std::format("Phenotype \"{}\" has no color. A random one will be generated for it.", this->get_identifier()));
		this->color = random::get()->generate_color();
	}

	if (this->get_species() != nullptr && !vector::contains(this->get_species()->get_phenotypes(), this)) {
		this->species->add_phenotype(this);
	}

	named_data_entry::initialize();
}

void phenotype::check() const
{
	if (!this->get_color().isValid()) {
		throw std::runtime_error(std::format("Phenotype \"{}\" has no color.", this->get_identifier()));
	}

	if (this->get_species() == nullptr) {
		throw std::runtime_error(std::format("Phenotype \"{}\" has no species.", this->get_identifier()));
	}
}

}
