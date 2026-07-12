#include "metternich.h"

#include "map/region_history.h"

#include "culture/culture.h"
#include "map/province.h"
#include "map/province_container.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "population/population_type.h"
#include "species/phenotype.h"
#include "technology/technological_period.h"
#include "technology/technology.h"
#include "technology/technology_category.h"
#include "technology/technology_subcategory.h"
#include "util/decimal_int.h"
#include "util/vector_util.h"

namespace metternich {

void region_history::process_gsml_scope(const gsml_data &scope, const QDate &date)
{
	const std::string &tag = scope.get_tag();

	if (tag == "culture_weights") {
		this->culture_weights.clear();

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const culture *culture = culture::get(key);

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
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key_str = property.get_key();
			const population_group_key key(key_str);

			if (key.type != nullptr && !key.type->is_enabled()) {
				return;
			}

			const std::string &value = property.get_value();
			const int64_t population = std::stoll(value);

			if (population == 0) {
				this->population_groups.erase(key);
			} else {
				this->population_groups[key] = population;
			}
		});
	} else if (tag == "technology_category_period") {
		const technology_category *category = technology_category::get(scope.get_property_value("category"));
		const technological_period *period = technological_period::get(scope.get_property_value("period"));

		for (const technology *technology : category->get_technologies()) {
			if (technology->get_period()->get_start_year() <= period->get_start_year()) {
				if (!vector::contains(this->get_technologies(), technology)) {
					this->technologies.push_back(technology);
				}
			}
		}
	} else if (tag == "technology_subcategory_period") {
		const technology_subcategory *subcategory = technology_subcategory::get(scope.get_property_value("subcategory"));
		const technological_period *period = technological_period::get(scope.get_property_value("period"));

		for (const technology *technology : subcategory->get_technologies()) {
			if (technology->get_period()->get_start_year() <= period->get_start_year()) {
				if (!vector::contains(this->get_technologies(), technology)) {
					this->technologies.push_back(technology);
				}
			}
		}
	} else {
		data_entry_history::process_gsml_scope(scope, date);
	}
}

void region_history::distribute_population()
{
	if (this->get_literacy_rate() != 0 || !this->get_culture_weights().empty() || !this->get_phenotype_weights().empty()) {
		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			province_history *province_history = province->get_history();

			if (province_history->get_culture_weights().empty() && !this->get_culture_weights().empty()) {
				province_history->set_culture_weights(this->get_culture_weights());
			}

			if (province_history->get_phenotype_weights().empty() && !this->get_phenotype_weights().empty()) {
				province_history->set_phenotype_weights(this->get_phenotype_weights());
			}

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
		int unpopulated_site_count = 0;
		province_map<int> populatable_site_counts_by_province;

		//subtract the predefined population of provinces in the region from that of the region
		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			int province_populatable_site_count = 0;
			int64_t province_total_populatable_site_group_population = 0;

			for (const site *site : province->get_game_data()->get_sites()) {
				if (!site->get_game_data()->can_have_population() || !site->get_game_data()->is_built()) {
					continue;
				}

				province_total_populatable_site_group_population += site->get_history()->get_group_population(group_key);
				++province_populatable_site_count;
			}

			province_populatable_site_count = std::max(province_populatable_site_count, 1);

			populatable_site_counts_by_province[province] = province_populatable_site_count;

			if (province_populatable_site_count == 0) {
				continue;
			}

			const province_history *province_history = province->get_history();
			const int64_t province_group_population = std::max(std::max(province_history->get_group_population(group_key), province_history->get_lower_bound_group_population(group_key)), province_total_populatable_site_group_population);

			if (province_group_population != 0) {
				remaining_population -= province_group_population;
			}
			
			if (province_history->get_group_population(group_key) == 0) {
				unpopulated_site_count += province_populatable_site_count;
			}
		}

		if (remaining_population <= 0 || unpopulated_site_count == 0) {
			continue;
		}

		//apply the remaining population to provinces without a predefined population in history
		const int64_t population_per_site = remaining_population / unpopulated_site_count;

		for (const province *province : this->region->get_provinces()) {
			if (province->is_water_zone()) {
				continue;
			}

			const int province_populatable_site_count = populatable_site_counts_by_province[province];
			if (province_populatable_site_count == 0) {
				continue;
			}

			province_history *province_history = province->get_history();

			if (province_history->get_group_population(group_key) == 0) {
				const int64_t group_population = population_per_site * province_populatable_site_count;
				province_history->set_group_population(group_key, group_population);
			}
		}
	}
}

void region_history::apply_to_provinces() const
{
	if (this->get_technologies().empty()) {
		return;
	}

	for (const province *province : this->region->get_provinces()) {
		if (province->is_water_zone()) {
			continue;
		}

		for (const technology *technology : this->get_technologies()) {
			if (!vector::contains(province->get_history()->get_technologies(), technology)) {
				province->get_history()->add_technology(technology);
			}
		}
	}
}

}
