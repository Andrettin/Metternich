#include "metternich.h"

#include "map/tile.h"

#include "domain/domain.h"
#include "economy/resource.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/terrain_type.h"

namespace metternich {

tile::tile(const terrain_type *terrain) : terrain(terrain)
{
}

void tile::set_terrain(const terrain_type *terrain)
{
	if (terrain == this->get_terrain()) {
		return;
	}

	this->terrain = terrain;
}

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

const metternich::site *tile::get_settlement() const
{
	if (this->get_site() != nullptr && this->get_site()->is_settlement()) {
		return this->get_site();
	}

	return nullptr;
}

const holding_type *tile::get_holding_type() const
{
	const metternich::site *settlement = this->get_settlement();
	if (settlement != nullptr) {
		return settlement->get_game_data()->get_holding_type();
	}

	return nullptr;
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

const commodity_map<centesimal_int> &tile::get_commodity_outputs() const
{
	static commodity_map<centesimal_int> empty_map;

	if (this->get_site() != nullptr) {
		return this->get_site()->get_game_data()->get_commodity_outputs();
	}

	return empty_map;
}

void tile::calculate_commodity_outputs()
{
	if (this->get_site() != nullptr) {
		this->get_site()->get_game_data()->calculate_commodity_outputs();
	}
}

bool tile::produces_commodity(const commodity *commodity) const
{
	if (this->get_site() != nullptr) {
		return this->get_site()->get_game_data()->produces_commodity(commodity);
	}

	return false;
}

}
