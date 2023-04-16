#include "metternich.h"

#include "unit/civilian_unit.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/tile.h"
#include "unit/civilian_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/point_util.h"

namespace metternich {

civilian_unit::civilian_unit(const civilian_unit_type *type, const country *owner, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype)
	: type(type), owner(owner), population_type(population_type), culture(culture), religion(religion), phenotype(phenotype)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_population_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	connect(this, &civilian_unit::type_changed, this, &civilian_unit::icon_changed);
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
			} else if (this->improvement_under_construction != nullptr) {
				map::get()->set_tile_improvement(this->get_tile_pos(), this->improvement_under_construction);
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

	if (this->get_type()->is_explorer() && this->can_explore_tile(tile_pos)) {
		this->exploring = true;
		this->set_task_completion_turns(civilian_unit::exploration_turns);
		return;
	}

	const improvement *buildable_improvement = this->get_buildable_resource_improvement_for_tile(tile_pos);
	if (buildable_improvement != nullptr) {
		this->build_improvement(buildable_improvement);
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

bool civilian_unit::can_build_improvement(const improvement *improvement) const
{
	if (improvement->get_required_technology() != nullptr && !this->get_owner()->get_game_data()->has_technology(improvement->get_required_technology())) {
		return false;
	}

	return true;
}

bool civilian_unit::can_build_improvement_on_tile(const improvement *improvement, const QPoint &tile_pos) const
{
	if (!this->can_build_improvement(improvement)) {
		return false;
	}

	const tile *tile = map::get()->get_tile(tile_pos);
	return improvement->is_buildable_on_tile(tile);
}

void civilian_unit::build_improvement(const improvement *improvement)
{
	this->improvement_under_construction = improvement;

	//FIXME: set the task completion turns as a field for each improvement?
	this->set_task_completion_turns(civilian_unit::improvement_construction_turns);
}

void civilian_unit::cancel_work()
{
	this->set_task_completion_turns(0);
	this->improvement_under_construction = nullptr;
	this->exploring = false;
}

const improvement *civilian_unit::get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_resource() == nullptr) {
		return nullptr;
	}

	for (const improvement *improvement : improvement::get_all()) {
		if (improvement->get_resource() == nullptr) {
			continue;
		}

		if (!this->can_build_improvement_on_tile(improvement, tile_pos)) {
			continue;
		}

		return improvement;
	}

	return nullptr;
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

void civilian_unit::disband(const bool restore_population_unit)
{
	if (this->is_working()) {
		this->cancel_work();
	}

	tile *tile = this->get_tile();

	assert_throw(tile != nullptr);

	map::get()->set_tile_civilian_unit(this->get_tile_pos(), nullptr);

	if (restore_population_unit) {
		this->get_owner()->get_game_data()->create_population_unit(this->get_population_type(), this->get_culture(), this->get_religion(), this->get_phenotype());
	}

	this->get_owner()->get_game_data()->remove_civilian_unit(this);
}

void civilian_unit::disband()
{
	this->disband(true);
}

}
