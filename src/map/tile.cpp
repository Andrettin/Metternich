#include "metternich.h"

#include "map/tile.h"

#include "map/province.h"
#include "map/province_game_data.h"

namespace metternich {

const domain *tile::get_owner() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	return this->get_province()->get_game_data()->get_owner();
}

}
