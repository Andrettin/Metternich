#include "metternich.h"

#include "infrastructure/building_slot.h"

#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/assert_util.h"

namespace metternich {

building_slot::building_slot(const building_slot_type *type, const metternich::province *province)
	: type(type), province(province)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_province() != nullptr);
}

void building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	if (building != nullptr) {
		assert_throw(building->get_building_class()->get_slot_type() == this->get_type());
	}

	if (this->get_building() != nullptr) {
		this->get_province()->get_game_data()->change_score(-this->get_building()->get_score());
	}

	this->building = building;

	if (this->get_building() != nullptr) {
		this->get_province()->get_game_data()->change_score(this->get_building()->get_score());
	}

	if (game::get()->is_running()) {
		this->get_province()->get_game_data()->reassign_workers();

		emit building_changed();
	}
}

int building_slot::get_employment_capacity() const
{
	if (this->get_building() != nullptr) {
		return this->get_building()->get_employment_capacity();
	}

	return 0;
}

}
