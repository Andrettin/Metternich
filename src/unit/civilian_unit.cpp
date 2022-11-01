#include "metternich.h"

#include "unit/civilian_unit.h"

#include "country/country.h"
#include "country/country_game_data.h"
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

void civilian_unit::cancel_move()
{
	assert_throw(map::get()->contains(this->original_tile_pos));

	if (map::get()->get_tile(this->original_tile_pos)->get_civilian_unit() != nullptr) {
		//cannot move back if the original tile is currently occupied by a different civilian unit
		return;
	}

	this->set_tile_pos(this->original_tile_pos);
	this->set_original_tile_pos(QPoint(-1, -1));
}

void civilian_unit::do_turn()
{
	if (this->is_moving()) {
		this->set_original_tile_pos(QPoint(-1, -1));
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
