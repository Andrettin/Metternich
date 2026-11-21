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
	if (province == nullptr) {
		this->province_index = tile::no_province_index;
	} else {
		this->province_index = static_cast<tile::province_index_type>(province->get_index());
	}
}

const domain *tile::get_owner() const
{
	const province *province = this->get_province();
	if (province == nullptr) {
		return nullptr;
	}

	return province->get_game_data()->get_owner();
}

}
