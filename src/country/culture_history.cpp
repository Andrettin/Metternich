#include "metternich.h"

#include "country/culture_history.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/region.h"

namespace metternich {

void culture_history::apply_to_country(const country *country) const
{
	country_game_data *country_game_data = country->get_game_data();

	for (const technology *technology : this->get_technologies()) {
		country_game_data->add_technology_with_prerequisites(technology);
	}

	for (const province *province : this->get_explored_provinces()) {
		if (!province->get_game_data()->is_on_map()) {
			continue;
		}

		country_game_data->explore_province(province);
	}

	for (const region *region : this->get_explored_regions()) {
		for (const province *province : region->get_provinces()) {
			if (!province->get_game_data()->is_on_map()) {
				continue;
			}

			country_game_data->explore_province(province);
		}
	}
}

}
