#include "metternich.h"

#include "culture/culture_history.h"

#include "domain/country_technology.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/region.h"

namespace metternich {

void culture_history::apply_to_domain(const domain *domain) const
{
	domain_game_data *domain_game_data = domain->get_game_data();

	for (const province *province : this->get_explored_provinces()) {
		if (!province->get_game_data()->is_on_map()) {
			continue;
		}

		domain_game_data->explore_province(province);
	}

	for (const region *region : this->get_explored_regions()) {
		for (const province *province : region->get_provinces()) {
			if (!province->get_game_data()->is_on_map()) {
				continue;
			}

			domain_game_data->explore_province(province);
		}
	}
}

void culture_history::apply_to_province(const province *province) const
{
	province_game_data *province_game_data = province->get_game_data();

	for (const technology *technology : this->get_technologies()) {
		province_game_data->add_technology_with_prerequisites(technology);
	}
}

}
