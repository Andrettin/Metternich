#include "metternich.h"

#include "map/province_history.h"

#include "database/gsml_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"

namespace metternich {

void province_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "population_groups") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key_str = property.get_key();
			const population_group_key key(key_str);

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
