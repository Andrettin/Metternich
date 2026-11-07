#include "metternich.h"

#include "map/tile.h"

#include "domain/domain.h"
#include "economy/resource.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"

namespace metternich {

const domain *tile::get_owner() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	return this->get_province()->get_game_data()->get_owner();
}

void tile::set_site(const metternich::site *site)
{
	if (site == this->get_site()) {
		return;
	}

	this->site = site;
}

const metternich::resource *tile::get_resource() const
{
	if (this->get_site() != nullptr) {
		return this->get_site()->get_map_data()->get_resource();
	}

	return nullptr;
}

bool tile::is_resource_discovered() const
{
	if (this->get_site() != nullptr) {
		return this->get_site()->get_game_data()->is_resource_discovered();
	}

	return false;
}

}
