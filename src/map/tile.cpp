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
#include "map/site_game_data.h"
#include "map/site_map_data.h"
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
	if (!base_terrain->get_subtiles().empty()) {
		std::array<const std::vector<int> *, 4> terrain_subtiles{};

		for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
			terrain_subtiles[i] = &base_terrain->get_subtiles();
		}

		for (size_t i = 0; i < terrain_subtiles.size(); ++i) {
			const short terrain_subtile = static_cast<short>(vector::get_random(*terrain_subtiles[i]));

			this->base_subtile_frames[i] = terrain_subtile;
		}
	} else {
		this->base_tile_frame = static_cast<short>(vector::get_random(base_terrain->get_tiles()));
	}
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
	if (this->get_site() != nullptr && this->get_site()->is_settlement()) {
		return this->get_site();
	}

	return nullptr;
}

const settlement_type *tile::get_settlement_type() const
{
	const metternich::site *settlement = this->get_settlement();
	if (settlement != nullptr) {
		return settlement->get_game_data()->get_settlement_type();
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

void tile::on_main_improvement_changed()
{
	const improvement *main_improvement = this->get_site()->get_game_data()->get_main_improvement();

	if (main_improvement != nullptr) {
		if (main_improvement->get_variation_count() > 1) {
			this->improvement_variation = random::get()->generate(main_improvement->get_variation_count());
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

bool tile::is_river_crossing_direction(const direction direction) const
{
	switch (direction) {
		case direction::north:
		case direction::south:
			return vector::contains(this->get_river_directions(), direction::west) && vector::contains(this->get_river_directions(), direction::east);
		case direction::west:
		case direction::east:
			return vector::contains(this->get_river_directions(), direction::west) && vector::contains(this->get_river_directions(), direction::east);
		default:
			assert_throw(false);
	}

	return false;
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

const pathway *tile::get_best_pathway() const
{
	if (!this->has_route()) {
		return nullptr;
	}

	const pathway *best_pathway = nullptr;

	for (const auto &[pathway, frame] : this->get_pathway_frames()) {
		if (best_pathway == nullptr || pathway->get_transport_level() > best_pathway->get_transport_level()) {
			best_pathway = pathway;
		}
	}

	return best_pathway;
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
