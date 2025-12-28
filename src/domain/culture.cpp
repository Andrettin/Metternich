#include "metternich.h"

#include "domain/culture.h"

#include "domain/cultural_group.h"
#include "infrastructure/building_class.h"
#include "population/population_class.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/random.h"
#include "util/vector_util.h"

namespace metternich {

culture::culture(const std::string &identifier) : culture_base(identifier)
{
}

culture::~culture()
{
}

void culture::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "species") {
		for (const std::string &value : values) {
			metternich::species *species = species::get(value);
			species->add_culture(this);
			this->species.push_back(species);
		}
	} else if (tag == "derived_cultures") {
		for (const std::string &value : values) {
			this->derived_cultures.push_back(culture::get(value));
		}
	} else if (tag == "derivation_conditions") {
		auto conditions = std::make_unique<and_condition<population_unit>>();
		conditions->process_gsml_data(scope);
		this->derivation_conditions = std::move(conditions);
	} else if (tag == "character_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		modifier->process_gsml_data(scope);
		this->character_modifier = std::move(modifier);
	} else {
		culture_base::process_gsml_scope(scope);
	}
}

void culture::initialize()
{
	if (!this->color.isValid()) {
		log::log_error(std::format("Culture \"{}\" has no color. A random one will be generated for it.", this->get_identifier()));
		this->color = random::get()->generate_color();
	}

	culture_base::initialize();
}

void culture::check() const
{
	assert_throw(this->get_color().isValid());

	if (this->get_species().empty()) {
		throw std::runtime_error(std::format("Culture \"{}\" has no species set for it.", this->get_identifier()));
	}

	if (this->get_weighted_phenotypes().empty()) {
		throw std::runtime_error(std::format("Culture \"{}\" has no weighted phenotypes.", this->get_identifier()));
	}

	culture_base::check();
}

std::vector<const phenotype *> culture::get_weighted_phenotypes() const
{
	phenotype_map<int64_t> phenotype_weights = this->get_phenotype_weights();

	std::erase_if(phenotype_weights, [this](const auto &element) {
		const auto &[key, value] = element;
		return !vector::contains(this->get_species(), key->get_species());
	});

	if (!phenotype_weights.empty()) {
		return archimedes::map::to_weighted_vector(phenotype_weights);
	}

	if (this->get_default_phenotype() != nullptr) {
		return { this->get_default_phenotype() };
	}

	std::vector<const phenotype *> weighted_phenotypes;

	for (const metternich::species *species : this->get_species()) {
		vector::merge(weighted_phenotypes, species->get_phenotypes());
	}

	return weighted_phenotypes;
}

}
