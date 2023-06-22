#include "metternich.h"

#include "map/tile.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "map/direction.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_adjacency_type.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/random.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

tile::tile(const terrain_type *base_terrain, const terrain_type *terrain) : terrain(terrain)
{
	this->base_tile_frame = static_cast<short>(vector::get_random(base_terrain->get_tiles()));
}

void tile::set_terrain(const terrain_type *terrain)
{
	if (terrain == this->get_terrain()) {
		return;
	}

	this->terrain = terrain;
}

const country *tile::get_owner() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	return this->get_province()->get_game_data()->get_owner();
}

const metternich::site *tile::get_settlement() const
{
	if (this->get_site() != nullptr && this->get_site()->get_type() == site_type::settlement) {
		return this->get_site();
	}

	return nullptr;
}

void tile::set_resource(const metternich::resource *resource)
{
	if (resource == this->get_resource()) {
		return;
	}

	this->resource = resource;

	if (resource == nullptr || resource->get_required_technology() != nullptr) {
		this->set_resource_discovered(false);
	} else {
		this->set_resource_discovered(true);
	}
}

void tile::set_improvement(const metternich::improvement *improvement)
{
	this->improvement = improvement;

	if (improvement != nullptr) {
		assert_throw(this->get_resource() == improvement->get_resource() || improvement->is_ruins());

		if (improvement->get_variation_count() > 1) {
			this->improvement_variation = random::get()->generate(improvement->get_variation_count());
		} else {
			this->improvement_variation = 0;
		}
	} else {
		this->improvement_variation = 0;
	}
}

void tile::add_river_direction(const direction direction)
{
	if (vector::contains(this->get_river_directions(), direction)) {
		return;
	}

	this->river_directions.push_back(direction);
}

void tile::set_direction_pathway(const direction direction, const pathway *pathway)
{
	const metternich::pathway *old_pathway = this->get_direction_pathway(direction);

	if (pathway == old_pathway) {
		return;
	}

	this->direction_pathways[static_cast<int>(direction)] = pathway;
}

void tile::calculate_pathway_frame(const pathway *pathway)
{
	assert_throw(pathway != nullptr);

	static constexpr size_t direction_count = static_cast<size_t>(direction::count);
	static_assert(direction_count == terrain_adjacency::direction_count);

	terrain_adjacency adjacency;

	for (size_t i = 0; i < direction_count; ++i) {
		const direction direction = static_cast<archimedes::direction>(i);
		
		if (this->get_direction_pathway(direction) == pathway) {
			adjacency.set_direction_adjacency_type(direction, terrain_adjacency_type::same);
		} else {
			adjacency.set_direction_adjacency_type(direction, terrain_adjacency_type::other);
		}
	}

	const int pathway_frame = defines::get()->get_route_adjacency_tile(adjacency);

	if (pathway_frame != -1) {
		this->set_pathway_frame(pathway, pathway_frame);
	}
}

void tile::calculate_pathway_frames()
{
	std::vector<const pathway *> pathways;

	for (const pathway *pathway : this->direction_pathways) {
		if (pathway == nullptr) {
			continue;
		}

		if (vector::contains(pathways, pathway)) {
			continue;
		}

		pathways.push_back(pathway);
	}

	for (const pathway *pathway : pathways) {
		this->calculate_pathway_frame(pathway);
	}
}

int tile::get_output_value() const
{
	if (this->get_improvement() == nullptr) {
		return 0;
	}

	const commodity *output_commodity = this->get_improvement()->get_output_commodity();

	if (output_commodity == nullptr) {
		return 0;
	}

	return this->get_improvement()->get_output_multiplier();
}

}
