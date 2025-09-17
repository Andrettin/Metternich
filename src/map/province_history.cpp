#include "metternich.h"

#include "map/province_history.h"

#include "database/gsml_data.h"
#include "domain/culture.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "population/population_type.h"
#include "species/phenotype.h"
#include "util/assert_util.h"
#include "util/decimal_int.h"
#include "util/map_util.h"
#include "util/vector_util.h"

namespace metternich {

void province_history::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "culture") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->culture_weights.clear();
		const metternich::culture *culture = culture::get(value);
		if (culture != nullptr) {
			this->culture_weights[culture] = 1;
		}
	} else if (key == "phenotype") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->phenotype_weights.clear();
		const metternich::phenotype *phenotype = phenotype::get(value);
		if (phenotype != nullptr) {
			this->phenotype_weights[phenotype] = 1;
		}
	} else {
		data_entry_history::process_gsml_property(property);
	}
}

void province_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "culture_weights") {
		this->culture_weights.clear();

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const metternich::culture *culture = culture::get(key);

			const std::string &value = property.get_value();
			const int64_t weight = decimal_int(value).get_value();

			this->culture_weights[culture] = weight;
		});
	} else if (tag == "phenotype_weights") {
		this->phenotype_weights.clear();

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const phenotype *phenotype = phenotype::get(key);

			const std::string &value = property.get_value();
			const int64_t weight = decimal_int(value).get_value();

			this->phenotype_weights[phenotype] = weight;
		});
	} else if (tag == "population_groups") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key_str = property.get_key();
			const population_group_key key(key_str);

			if (key.type != nullptr && !key.type->is_enabled()) {
				return;
			}

			const std::string &value = property.get_value();
			const int population = std::stoi(value);

			if (population == 0) {
				this->population_groups.erase(key);
			} else {
				this->population_groups[key] = population;
			}
		});
	} else {
		data_entry_history::process_gsml_scope(scope);
	}
}

const culture *province_history::get_main_culture() const
{
	const culture *best_culture = nullptr;
	int64_t best_weight = 0;

	for (const auto &[culture, weight] : this->get_culture_weights()) {
		if (best_culture == nullptr || weight > best_weight) {
			best_culture = culture;
			best_weight = weight;
		}
	}

	return best_culture;
}

std::vector<const phenotype *> province_history::get_weighted_phenotypes_for_culture(const metternich::culture *culture) const
{
	assert_throw(culture != nullptr);

	phenotype_map<int64_t> phenotype_weights = this->get_phenotype_weights();

	std::erase_if(phenotype_weights, [culture](const auto & element) {
		const auto &[key, value] = element;
		return !vector::contains(culture->get_species(), key->get_species());
	});

	return archimedes::map::to_weighted_vector(phenotype_weights);
}

void province_history::initialize_population()
{
	//use the sum of the site historical populations for population groups which are missing in the province's history
	population_group_map<int> site_population_groups;

	for (const site *site : this->province->get_game_data()->get_sites()) {
		site_history *site_history = site->get_history();
		for (const auto &[group_key, population] : site_history->get_population_groups()) {
			site_population_groups[group_key] += population;
		}
	}

	for (const auto &[group_key, population] : site_population_groups) {
		this->lower_bound_population_groups[group_key] = std::max(this->lower_bound_population_groups[group_key], population);
	}
}

void province_history::distribute_population()
{
	for (const auto &[group_key, population] : this->lower_bound_population_groups) {
		this->population_groups[group_key] = std::max(this->population_groups[group_key], population);
	}

	for (const auto &[group_key, population] : this->population_groups) {
		if (population == 0) {
			continue;
		}

		int64_t remaining_population = population;
		int populatable_site_count = 0;

		//subtract the predefined population of populatable sites in the province from that of the province
		for (const site *site : this->province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			const site_history *site_history = site->get_history();
			const int site_group_population = site_history->get_group_population(group_key);

			if (site_group_population != 0) {
				remaining_population -= site_group_population;
			}

			++populatable_site_count;
		}

		if (remaining_population <= 0 || populatable_site_count == 0) {
			continue;
		}

		//apply the remaining population to settlements
		const int64_t population_per_settlement = remaining_population / populatable_site_count;

		if (population_per_settlement == 0) {
			continue;
		}

		for (const site *site : this->province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
				continue;
			}

			site_history *site_history = site->get_history();
			const int site_group_population = site_history->get_group_population(group_key);
			site_history->set_group_population(group_key, population_per_settlement + site_group_population);
		}
	}
}

}
