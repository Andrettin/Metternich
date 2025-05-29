#include "metternich.h"

#include "species/species.h"

#include "character/starting_age_category.h"
#include "script/modifier.h"
#include "species/geological_era.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"

#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

std::map<const taxon *, int> species::get_supertaxon_counts(const std::vector<const species *> &species_list, const std::vector<const taxon *> &taxons)
{
	std::map<const taxon *, int> supertaxon_counts;

	for (const species *species : species_list) {
		const taxon *supertaxon = species->get_supertaxon();
		while (supertaxon != nullptr) {
			supertaxon_counts[supertaxon]++;
			supertaxon = supertaxon->get_supertaxon();
		}
	}

	for (const taxon *taxon : taxons) {
		const metternich::taxon *supertaxon = taxon->get_supertaxon();
		while (supertaxon != nullptr) {
			supertaxon_counts[supertaxon]++;
			supertaxon = supertaxon->get_supertaxon();
		}
	}

	return supertaxon_counts;
}

std::vector<std::string> species::get_name_list(const std::vector<const species *> &source_species_list)
{
	static constexpr size_t max_species_names_size = 20;

	std::vector<const species *> species_list = source_species_list;
	std::vector<const taxon *> taxons;
	
	std::vector<std::string> species_names;
	for (const species *species : species_list) {
		species_names.push_back(species->get_name());
	}
	
	bool changed_name_list = true;
	std::map<const taxon *, int> supertaxon_counts;
	while (species_names.size() > max_species_names_size && changed_name_list == true) {
		changed_name_list = false;
		
		supertaxon_counts = get_supertaxon_counts(species_list, taxons);

		//get the taxon with lowest rank and highest count
		const taxon *best_taxon = nullptr;
		taxonomic_rank best_taxonomic_rank = taxonomic_rank::none;
		int best_value = 0;
		for (const auto &kv_pair : supertaxon_counts) {
			const taxon *taxon = kv_pair.first;
			const int value = kv_pair.second;
			if (value > 1) {
				const taxonomic_rank taxonomic_rank = taxon->get_rank();
				if (best_taxon == nullptr || taxonomic_rank < best_taxonomic_rank || (taxonomic_rank == best_taxonomic_rank && value > best_value)) {
					best_taxon = taxon;
					best_taxonomic_rank = taxonomic_rank;
					best_value = value;
				}
			}
		}
		
		if (best_taxon != nullptr) {
			changed_name_list = true;
			
			std::vector<const species *> new_species_list;
			std::vector<const taxon *> new_taxons;
			
			for (const species *species : species_list) {
				if (!species->is_subtaxon_of(best_taxon)) {
					new_species_list.push_back(species);
				}
			}
			for (const taxon *taxon : taxons) {
				if (!taxon->is_subtaxon_of(best_taxon)) {
					new_taxons.push_back(taxon);
				}
			}
			new_taxons.push_back(best_taxon);
			
			species_list = new_species_list;
			taxons = new_taxons;
			
			species_names.clear();
			
			for (const species *species : species_list) {
				species_names.push_back(species->get_name());
			}
			for (const taxon *taxon : taxons) {
				species_names.push_back(taxon->get_common_name());
			}
		}
	}

	std::sort(species_names.begin(), species_names.end());

	return species_names;
}

species::species(const std::string &identifier)
	: taxon_base(identifier), era(geological_era::none)
{
}

species::~species()
{
}

void species::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "pre_evolutions") {
		for (const std::string &value : values) {
			species *other_species = species::get(value);
			this->pre_evolutions.push_back(other_species);
			other_species->evolutions.push_back(this);
		}
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const character>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		taxon_base::process_gsml_scope(scope);
	}
}

void species::check() const
{
	if (this->get_supertaxon() == nullptr) {
		throw std::runtime_error(std::format("Species \"{}\" has no supertaxon.", this->get_identifier()));
	}

	if (this->get_era() == geological_era::none && !this->is_ethereal()) {
		//throw std::runtime_error("Non-ethereal species \"" + this->get_identifier() + "\" has no era.");
	}

	for (const species *pre_evolution : this->get_pre_evolutions()) {
		if (this->get_era() != geological_era::none && pre_evolution->get_era() != geological_era::none && this->get_era() <= pre_evolution->get_era()) {
			throw std::runtime_error("Species \"" + this->get_identifier() + "\" is set to evolve from \"" + pre_evolution->get_identifier() + "\", but is from the same or an earlier era than the latter.");
		}
	}

	if (this->is_sapient() && this->get_phenotypes().empty()) {
		throw std::runtime_error(std::format("Sapient species \"{}\" has no phenotypes.", this->get_identifier()));
	}

	if (this->is_sapient()) {
		magic_enum::enum_for_each<starting_age_category>([&](const starting_age_category category) {
			if (category == starting_age_category::none) {
				return;
			}

			if (this->get_starting_age_modifier(category).is_null()) {
				throw std::runtime_error(std::format("Sapient species \"{}\" has no starting age modifier for starting age category \"{}\".", this->get_identifier(), magic_enum::enum_name(category)));
			}
		});

		if (this->get_cultures().empty()) {
			throw std::runtime_error(std::format("Sapient species \"{}\" has no cultures set for it.", this->get_identifier()));
		}
	}
}

taxonomic_rank species::get_rank() const
{
	return taxonomic_rank::species;
}

std::string species::get_scientific_name() const
{
	if (this->get_supertaxon() == nullptr) {
		throw std::runtime_error("Cannot get the scientific name for species \"" + this->get_identifier() + "\", as it has no supertaxon.");
	}

	if (this->get_supertaxon()->get_rank() != taxonomic_rank::genus) {
		throw std::runtime_error("Cannot get the scientific name for species \"" + this->get_identifier() + "\", as its supertaxon is not a genus.");
	}

	if (!this->get_specific_name().empty()) {
		return this->get_supertaxon()->get_name() + " " + this->get_specific_name();
	}

	return this->get_supertaxon()->get_name();
}

bool species::is_prehistoric() const
{
	return this->get_era() < geological_era::holocene;
}

}