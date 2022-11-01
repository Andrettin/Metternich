#include "metternich.h"

#include "unit/civilian_unit.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/tile.h"
#include "unit/civilian_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

civilian_unit::civilian_unit(const civilian_unit_type *type, const metternich::country *owner)
	: type(type), owner(owner)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);

	connect(this, &civilian_unit::type_changed, this, &civilian_unit::icon_changed);
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

	return tile->get_owner() == this->get_owner();
}

void civilian_unit::move_to(const QPoint &tile_pos)
{
	this->set_original_tile_pos(this->get_tile_pos());
	this->set_tile_pos(tile_pos);

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

const improvement *civilian_unit::get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const
{
	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_resource() == nullptr) {
		return nullptr;
	}

	for (const improvement *improvement : improvement::get_all()) {
		if (improvement->get_resource() != tile->get_resource()) {
			continue;
		}

		if (tile->get_improvement() != nullptr) {
			if (improvement == tile->get_improvement()) {
				continue;
			}

			if (improvement->get_output_value() <= tile->get_improvement()->get_output_value()) {
				continue;
			}
		}

		return improvement;
	}

	return nullptr;
}

void civilian_unit::build_improvement(const improvement *improvement)
{
	this->improvement_under_construction = improvement;

	//FIXME: set the task completion turns depending on the work to be done
	this->set_task_completion_turns(2);
}

void civilian_unit::cancel_work()
{
	this->set_task_completion_turns(0);
	this->improvement_under_construction = nullptr;
}

void civilian_unit::do_turn()
{
	if (this->is_moving()) {
		this->set_original_tile_pos(QPoint(-1, -1));
	}

	if (this->task_completion_turns > 0) {
		this->set_task_completion_turns(this->task_completion_turns - 1);

		if (this->task_completion_turns == 0) {
			if (this->improvement_under_construction != nullptr) {
				map::get()->set_tile_improvement(this->get_tile_pos(), this->improvement_under_construction);
				this->improvement_under_construction = nullptr;
			}
		}
	}
}

void civilian_unit::disband()
{
	tile *tile = this->get_tile();

	assert_throw(tile != nullptr);

	tile->set_civilian_unit(nullptr);
	this->get_owner()->get_game_data()->remove_civilian_unit(this);
}

}
