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
		this->get_tile()->set_civilian_unit(nullptr);
	}

	this->tile_pos = tile_pos;

	if (this->get_tile() != nullptr) {
		this->get_tile()->set_civilian_unit(this);
	}
}

tile *civilian_unit::get_tile() const
{
	if (this->get_tile_pos() == QPoint(-1, -1)) {
		return nullptr;
	}

	return map::get()->get_tile(tile_pos);
}

void civilian_unit::disband()
{
	tile *tile = this->get_tile();

	assert_throw(tile != nullptr);

	tile->set_civilian_unit(nullptr);
	this->get_owner()->get_game_data()->remove_civilian_unit(this);
}

}
