#include "metternich.h"

#include "map/region_history.h"

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "util/vector_util.h"

namespace metternich {

void region_history::process_gsml_scope(const gsml_data &scope)
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

void region_history::distribute_population()
{
	if (this->get_literacy_rate() != 0) {
		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			province_history *province_history = province->get_history();

			if (province_history->get_literacy_rate() == 0) {
				province_history->set_literacy_rate(this->get_literacy_rate());
			}
		}
	}

	for (const auto &[group_key, population] : this->population_groups) {
		if (population == 0) {
			continue;
		}

		int64_t remaining_population = population;
		int unpopulated_settlement_count = 0;

		//subtract the predefined population of provinces in the region from that of the region
		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			if (province->get_game_data()->get_settlement_count() == 0) {
				continue;
			}

			int province_total_settlement_group_population = 0;

			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				province_total_settlement_group_population += settlement->get_history()->get_group_population(group_key);
			}

			const province_history *province_history = province->get_history();
			const int province_group_population = std::max(province_history->get_group_population(group_key), province_total_settlement_group_population);

			if (province_group_population != 0) {
				remaining_population -= province_group_population;
			}
			
			if (province_history->get_group_population(group_key) == 0) {
				unpopulated_settlement_count += province->get_game_data()->get_settlement_count();
			}
		}

		if (remaining_population <= 0 || unpopulated_settlement_count == 0) {
			continue;
		}

		//apply the remaining population to provinces without a predefined population in history
		const int64_t population_per_settlement = remaining_population / unpopulated_settlement_count;

		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			if (province->get_game_data()->get_settlement_count() == 0) {
				continue;
			}

			province_history *province_history = province->get_history();

			if (province_history->get_group_population(group_key) == 0) {
				province_history->set_group_population(group_key, population_per_settlement * province->get_game_data()->get_settlement_count());
			}
		}
	}
}

}
