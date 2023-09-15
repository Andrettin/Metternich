#include "metternich.h"

#include "map/province_history.h"

#include "database/gsml_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
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

void province_history::distribute_population()
{
	for (const auto &[group_key, population] : this->population_groups) {
		if (population == 0) {
			continue;
		}

		int64_t remaining_population = population;
		int unpopulated_settlement_count = 0;

		//subtract the predefined population of settlements in the province from that of the province
		for (const site *settlement : this->province->get_game_data()->get_settlement_sites()) {
			const site_history *settlement_history = settlement->get_history();
			const int settlement_group_population = settlement_history->get_group_population(group_key);

			if (settlement_group_population == 0) {
				++unpopulated_settlement_count;
			} else {
				remaining_population -= settlement_group_population;
			}
		}

		if (remaining_population <= 0 || unpopulated_settlement_count == 0) {
			return;
		}

		//apply the remaining population to settlements
		const int64_t population_per_settlement = remaining_population / unpopulated_settlement_count;

		for (const site *settlement : this->province->get_game_data()->get_settlement_sites()) {
			site_history *settlement_history = settlement->get_history();
			const int settlement_group_population = settlement_history->get_group_population(group_key);
			settlement_history->set_group_population(group_key, population_per_settlement + settlement_group_population);
		}
	}
}

}
