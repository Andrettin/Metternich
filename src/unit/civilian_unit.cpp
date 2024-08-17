#include "metternich.h"

#include "unit/civilian_unit.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "economy/resource.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "unit/civilian_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/vector_util.h"

namespace metternich {

civilian_unit::civilian_unit(const civilian_unit_type *type, const country *owner, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const site *home_settlement)
	: type(type), owner(owner), culture(culture), religion(religion), phenotype(phenotype), home_settlement(home_settlement)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_home_settlement() != nullptr);

	connect(this, &civilian_unit::type_changed, this, &civilian_unit::icon_changed);

	connect(this->get_owner()->get_game_data(), &country_game_data::provinces_changed, this, &civilian_unit::improvable_resources_changed);
	connect(this->get_owner()->get_game_data(), &country_game_data::commodity_outputs_changed, this, &civilian_unit::improvable_resources_changed);
	connect(this->get_owner()->get_game_data(), &country_game_data::technologies_changed, this, &civilian_unit::improvable_resources_changed);

	connect(this->get_owner()->get_game_data(), &country_game_data::provinces_changed, this, &civilian_unit::prospectable_tiles_changed);
	connect(this->get_owner()->get_game_data(), &country_game_data::prospected_tiles_changed, this, &civilian_unit::prospectable_tiles_changed);
	connect(this->get_owner()->get_game_data(), &country_game_data::technologies_changed, this, &civilian_unit::prospectable_tiles_changed);
}

civilian_unit::civilian_unit(const civilian_unit_type *type, const country *owner, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const site *home_settlement)
	: civilian_unit(type, owner, culture, religion, phenotype, home_settlement)
{
	this->population_type = population_type;
	assert_throw(this->get_population_type() != nullptr);
}

civilian_unit::civilian_unit(const metternich::character *character, const country *owner)
	: civilian_unit(character->get_civilian_unit_type(), owner, character->get_culture(), character->get_religion(), character->get_phenotype(), character->get_home_settlement())
{
	this->character = character;

	character_game_data *character_game_data = this->get_character()->get_game_data();
	character_game_data->set_civilian_unit(this);
}

void civilian_unit::do_turn()
{
	if (this->is_moving()) {
		this->set_original_tile_pos(QPoint(-1, -1));
	}

	if (this->task_completion_turns > 0) {
		this->set_task_completion_turns(this->task_completion_turns - 1);

		if (this->task_completion_turns == 0) {
			if (this->exploring) {
				//set the adjacent tiles to explored for the owner player
				point::for_each_adjacent(this->get_tile_pos(), [this](const QPoint &adjacent_pos) {
					if (!map::get()->contains(adjacent_pos)) {
						return;
					}

					if (!this->get_owner()->get_game_data()->is_tile_explored(adjacent_pos)) {
						this->get_owner()->get_game_data()->explore_tile(adjacent_pos);
					}
				});

				this->exploring = false;
			}
			
			if (this->prospecting) {
				this->get_owner()->get_game_data()->prospect_tile(this->get_tile_pos());
				this->prospecting = false;
			}
			
			if (this->improvement_under_construction != nullptr) {
				this->get_tile()->get_site()->get_game_data()->set_improvement(this->improvement_under_construction->get_slot(), this->improvement_under_construction);
				this->improvement_under_construction = nullptr;
			}
		}
	}
}

void civilian_unit::do_ai_turn()
{
	if (this->is_busy()) {
		return;
	}

	for (const province *province : this->get_owner()->get_game_data()->get_provinces()) {
		for (const QPoint &resource_tile_pos : province->get_game_data()->get_resource_tiles()) {
			if (resource_tile_pos != this->get_tile_pos() && !this->can_move_to(resource_tile_pos)) {
				continue;
			}

			const improvement *improvement = this->get_buildable_resource_improvement_for_tile(resource_tile_pos);
			if (improvement == nullptr) {
				continue;
			}

			if (resource_tile_pos == this->get_tile_pos()) {
				this->build_improvement(improvement);
				return;
			} else {
				this->move_to(resource_tile_pos);
				return;
			}
		}
	}
}

const icon *civilian_unit::get_icon() const
{
	return this->get_type()->get_icon();
}

void civilian_unit::set_tile_pos(const QPoint &tile_pos)
{
	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	if (this->get_tile() != nullptr) {
		map::get()->set_tile_civilian_unit(this->get_tile_pos(), nullptr);
	}

	this->tile_pos = tile_pos;

	if (this->get_tile() != nullptr) {
		map::get()->set_tile_civilian_unit(this->get_tile_pos(), this);
	}

	emit tile_pos_changed();
}

tile *civilian_unit::get_tile() const
{
	if (this->get_tile_pos() == QPoint(-1, -1)) {
		return nullptr;
	}

	return map::get()->get_tile(tile_pos);
}

bool civilian_unit::can_move_to(const QPoint &tile_pos) const
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_civilian_unit() != nullptr) {
		return false;
	}

	if (tile->get_owner() == this->get_owner()) {
		return true;
	}

	if (tile->get_owner() != nullptr) {
		return tile->get_owner()->get_game_data()->is_any_vassal_of(this->get_owner());
	}

	return false;
}

void civilian_unit::move_to(const QPoint &tile_pos)
{
	this->set_original_tile_pos(this->get_tile_pos());
	this->set_tile_pos(tile_pos);

	if (this->get_type()->is_explorer() && this->can_explore_tile(tile_pos) && this->get_type()->is_prospector() && this->can_prospect_tile(tile_pos)) {
		//explore and prospect at the same time
		this->exploring = true;
		this->prospecting = true;
		this->set_task_completion_turns(std::max(civilian_unit::exploration_turns, civilian_unit::prospection_turns));
		return;
	}

	if (this->get_type()->is_explorer() && this->can_explore_tile(tile_pos)) {
		this->exploring = true;
		this->set_task_completion_turns(civilian_unit::exploration_turns);
		return;
	}

	if (this->get_type()->is_prospector() && this->can_prospect_tile(tile_pos)) {
		this->prospecting = true;
		this->set_task_completion_turns(civilian_unit::prospection_turns);
		return;
	}

	if (this->can_build_on_tile()) {
		this->build_on_tile();
	}
}

void civilian_unit::cancel_move()
{
	assert_throw(map::get()->contains(this->original_tile_pos));

	if (map::get()->get_tile(this->original_tile_pos)->get_civilian_unit() != nullptr) {
		//cannot move back if the original tile is currently occupied by a different civilian unit
		return;
	}

	if (this->is_working()) {
		this->cancel_work();
	}

	this->set_tile_pos(this->original_tile_pos);
	this->set_original_tile_pos(QPoint(-1, -1));
}

bool civilian_unit::can_build_on_tile() const
{
	return this->get_buildable_resource_improvement_for_tile(this->get_tile_pos()) != nullptr;
}

void civilian_unit::build_on_tile()
{
	const improvement *buildable_improvement = this->get_buildable_resource_improvement_for_tile(this->get_tile_pos());
	assert_throw(buildable_improvement != nullptr);
	this->build_improvement(buildable_improvement);
}

bool civilian_unit::can_build_improvement(const improvement *improvement) const
{
	const country_game_data *country_game_data = this->get_owner()->get_game_data();
	if (improvement->get_required_technology() != nullptr && !country_game_data->has_technology(improvement->get_required_technology())) {
		return false;
	}

	if (improvement->get_resource() != nullptr && !this->get_type()->can_improve_resource(improvement->get_resource())) {
		return false;
	}

	if (improvement->get_wealth_cost() > 0 && improvement->get_wealth_cost() > country_game_data->get_wealth_with_credit()) {
		return false;
	}

	for (const auto &[commodity, cost] : improvement->get_commodity_costs()) {
		if (cost > country_game_data->get_stored_commodity(commodity)) {
			return false;
		}
	}

	return true;
}

bool civilian_unit::can_build_improvement_on_tile(const improvement *improvement, const QPoint &tile_pos) const
{
	if (!this->can_build_improvement(improvement)) {
		return false;
	}

	const tile *tile = map::get()->get_tile(tile_pos);
	const site *site = tile->get_site();

	if (site == nullptr) {
		return false;
	}

	return improvement->is_buildable_on_site(site);
}

void civilian_unit::build_improvement(const improvement *improvement)
{
	this->improvement_under_construction = improvement;

	//FIXME: set the task completion turns as a field for each improvement?
	this->set_task_completion_turns(civilian_unit::improvement_construction_turns);

	country_game_data *country_game_data = this->get_owner()->get_game_data();
	if (improvement->get_wealth_cost() > 0) {
		country_game_data->change_wealth_inflated(-improvement->get_wealth_cost());
	}

	for (const auto &[commodity, cost] : improvement->get_commodity_costs()) {
		country_game_data->change_stored_commodity(commodity, -cost);
	}
}

void civilian_unit::cancel_work()
{
	if (this->improvement_under_construction != nullptr) {
		country_game_data *country_game_data = this->get_owner()->get_game_data();
		if (this->improvement_under_construction->get_wealth_cost() > 0) {
			country_game_data->change_wealth(this->improvement_under_construction->get_wealth_cost());
		}

		for (const auto &[commodity, cost] : this->improvement_under_construction->get_commodity_costs()) {
			country_game_data->change_stored_commodity(commodity, cost);
		}
	}

	this->set_task_completion_turns(0);
	this->improvement_under_construction = nullptr;
	this->exploring = false;
	this->prospecting = false;
}

const improvement *civilian_unit::get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_resource() == nullptr) {
		return nullptr;
	}

	for (const improvement *improvement : tile->get_resource()->get_improvements()) {
		assert_throw(improvement->get_resource() != nullptr);

		if (!this->can_build_improvement_on_tile(improvement, tile_pos)) {
			continue;
		}

		return improvement;
	}

	return nullptr;
}

resource_map<std::vector<QPoint>> civilian_unit::get_improvable_resource_tiles() const
{
	resource_map<std::vector<QPoint>> resource_tiles;

	if (this->get_type()->get_improvable_resources().empty()) {
		return resource_tiles;
	}

	for (const province *province : this->get_owner()->get_game_data()->get_provinces()) {
		for (const QPoint &tile_pos : province->get_game_data()->get_resource_tiles()) {
			const tile *tile = map::get()->get_tile(tile_pos);

			if (tile->get_resource() == nullptr) {
				continue;
			}

			if (!this->get_type()->can_improve_resource(tile->get_resource())) {
				continue;
			}

			const improvement *improvement = this->get_buildable_resource_improvement_for_tile(tile_pos);
			if (improvement != nullptr) {
				resource_tiles[tile->get_resource()].push_back(tile_pos);
			}
		}
	}

	return resource_tiles;
}

QVariantList civilian_unit::get_improvable_resource_tiles_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_improvable_resource_tiles());
}

bool civilian_unit::can_explore_tile(const QPoint &tile_pos) const
{
	if (!this->get_type()->is_explorer()) {
		return false;
	}

	if (!this->get_owner()->get_game_data()->is_tile_explored(tile_pos)) {
		//can only explore already-explored tiles which border non-explored ones
		return false;
	}

	bool adjacent_unexplored = false;

	point::for_each_adjacent_until(tile_pos, [this, &adjacent_unexplored](const QPoint &adjacent_pos) {
		if (!map::get()->contains(adjacent_pos)) {
			return false;
		}

		if (!this->get_owner()->get_game_data()->is_tile_explored(adjacent_pos)) {
			adjacent_unexplored = true;
			return true;
		}

		return false;
	});

	return adjacent_unexplored;
}

bool civilian_unit::can_prospect_tile(const QPoint &tile_pos) const
{
	if (!this->get_type()->is_prospector()) {
		return false;
	}

	if (this->get_owner()->get_game_data()->is_tile_prospected(tile_pos)) {
		//already prospected
		return false;
	}

	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->is_resource_discovered()) {
		return false;
	}

	for (const resource *resource : resource::get_all()) {
		if (!resource->is_prospectable()) {
			continue;
		}

		if (!vector::contains(resource->get_terrain_types(), tile->get_terrain())) {
			continue;
		}

		if (resource->get_required_technology() != nullptr && !this->get_owner()->get_game_data()->has_technology(resource->get_required_technology())) {
			continue;
		}

		return true;
	}

	return false;
}

terrain_type_map<std::vector<QPoint>> civilian_unit::get_prospectable_tiles() const
{
	terrain_type_map<std::vector<QPoint>> prospectable_tiles;

	if (!this->get_type()->is_prospector()) {
		return prospectable_tiles;
	}

	terrain_type_set prospectable_terrains;

	for (const resource *resource : resource::get_all()) {
		if (!resource->is_prospectable()) {
			continue;
		}

		if (resource->get_required_technology() != nullptr && !this->get_owner()->get_game_data()->has_technology(resource->get_required_technology())) {
			continue;
		}

		for (const terrain_type *resource_terrain : resource->get_terrain_types()) {
			prospectable_terrains.insert(resource_terrain);
		}
	}

	for (const province *province : this->get_owner()->get_game_data()->get_provinces()) {
		bool has_prospectable_terrain = false;
		for (const terrain_type *prospectable_terrain : prospectable_terrains) {
			if (province->get_game_data()->get_tile_terrain_counts().contains(prospectable_terrain)) {
				has_prospectable_terrain = true;
				break;
			}
		}
		if (!has_prospectable_terrain) {
			continue;
		}

		for (const QPoint &tile_pos : province->get_map_data()->get_tiles()) {
			if (!this->can_prospect_tile(tile_pos)) {
				continue;
			}

			const tile *tile = map::get()->get_tile(tile_pos);
			prospectable_tiles[tile->get_terrain()].push_back(tile_pos);
		}
	}

	return prospectable_tiles;
}

QVariantList civilian_unit::get_prospectable_tiles_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_prospectable_tiles());
}

void civilian_unit::disband(const bool dead)
{
	if (this->is_working()) {
		this->cancel_work();
	}

	if (this->get_character() != nullptr) {
		character_game_data *character_game_data = this->get_character()->get_game_data();
		character_game_data->set_civilian_unit(nullptr);

		if (dead) {
			character_game_data->set_dead(true);
		}
	}

	tile *tile = this->get_tile();

	assert_throw(tile != nullptr);

	map::get()->set_tile_civilian_unit(this->get_tile_pos(), nullptr);

	if (!dead && this->get_population_type() != nullptr) {
		this->get_home_settlement()->get_game_data()->create_population_unit(this->get_population_type(), this->get_culture(), this->get_religion(), this->get_phenotype());
	}

	this->get_owner()->get_game_data()->remove_civilian_unit(this);
}

void civilian_unit::disband()
{
	this->disband(false);
}

}
