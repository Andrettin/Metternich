#include "metternich.h"

#include "culture/culture.h"

#include "culture/cultural_group.h"
#include "script/condition/and_condition.h"
#include "script/mean_time_to_happen.h"
#include "script/modifier.h"
#include "species/phenotype.h"
#include "species/species.h"
#include "util/assert_util.h"
#include "util/log_util.h"
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
	} else if (tag == "cultural_derivations") {
		assert_throw(values.empty());

		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			auto derivation = std::make_unique<cultural_derivation>(culture::get(child_tag));

			child_scope.for_each_child([this, &derivation](const gsml_data &grandchild_scope) {
				derivation->process_gsml_scope(grandchild_scope);
			});

			this->cultural_derivations.push_back(std::move(derivation));
		});
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

	if (this->get_phenotype_weights().empty()) {
		throw std::runtime_error(std::format("Culture \"{}\" has no phenotype weights.", this->get_identifier()));
	}

	if (this->get_language() == nullptr) {
		log::log_error(std::format("Culture \"{}\" has no language.", this->get_identifier()));
	}

	culture_base::check();
}

const phenotype_map<int64_t> culture::get_phenotype_weights() const
{
	phenotype_map<int64_t> phenotype_weights = culture_base::get_phenotype_weights();

	std::erase_if(phenotype_weights, [this](const auto &element) {
		const auto &[key, value] = element;
		return !vector::contains(this->get_species(), key->get_species());
	});

	if (phenotype_weights.empty()) {
		if (this->get_default_phenotype() != nullptr) {
			phenotype_weights[this->get_default_phenotype()] = 1;
		} else {
			for (const metternich::species *species : this->get_species()) {
				for (const phenotype *phenotype : species->get_phenotypes()) {
					++phenotype_weights[phenotype];
				}
			}
		}
	}

	return phenotype_weights;
}

culture::cultural_derivation::cultural_derivation(const metternich::culture *culture) : culture(culture)
{
}

culture::cultural_derivation::~cultural_derivation()
{
}

void culture::cultural_derivation::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<population_unit>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "mean_time_to_happen") {
		this->mean_time_to_happen = std::make_unique<metternich::mean_time_to_happen<population_unit>>();
		scope.process(this->mean_time_to_happen.get());
	} else {
		throw std::runtime_error(std::format("Invalid cultural derivation scope: \"{}\".", scope.get_tag()));
	}
}

}
