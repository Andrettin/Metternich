#include "metternich.h"

#include "map/tile.h"

#include "map/province.h"
#include "map/province_game_data.h"

namespace metternich {

metternich::province *tile::get_province() const
{
	if (this->province_index == tile::no_province_index) {
		return nullptr;
	}

	return province::get_all()[this->province_index];
}

void tile::set_province(metternich::province *province)
{
	this->province_index = static_cast<uint16_t>(province->get_index());
}

const domain *tile::get_owner() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	return this->get_province()->get_game_data()->get_owner();
}

}
