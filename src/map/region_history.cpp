#include "metternich.h"

#include "map/region_history.h"

#include "map/province.h"
#include "map/province_history.h"
#include "map/region.h"
#include "util/vector_util.h"

namespace metternich {

void region_history::distribute_population()
{
	int64_t population = this->get_population();

	if (population == 0) {
		return;
	}

	int unpopulated_province_count = 0;

	//subtract the predefined population of provinces in the region from that of the region
	for (const province *province : this->region->get_provinces()) {
		const province_history *province_history = province->get_history();

		if (province_history->get_population() != 0) {
			population -= province_history->get_population();
		} else {
			++unpopulated_province_count;
		}
	}

	if (population <= 0 || unpopulated_province_count == 0) {
		return;
	}

	//apply the remaining population to provinces without a predefined population in history
	const int64_t population_per_province = population / unpopulated_province_count;

	for (const province *province : this->region->get_provinces()) {
		province_history *province_history = province->get_history();

		if (province_history->get_population() == 0) {
			province_history->set_population(population_per_province);
		}
	}
}

}
