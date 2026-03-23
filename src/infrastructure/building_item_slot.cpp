#include "metternich.h"

#include "infrastructure/building_item_slot.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "domain/country_economy.h"
#include "domain/domain.h"
#include "item/item.h"
#include "item/item_creation_type.h"
#include "util/assert_util.h"
#include "util/dice.h"
#include "util/random.h"

namespace metternich {

building_item_slot::building_item_slot(const metternich::item_creation_type *item_creation_type, const metternich::building_slot *building_slot)
	: item_creation_type(item_creation_type), building_slot(building_slot)
{
	assert_throw(this->get_item_creation_type() != nullptr);
	assert_throw(this->get_building_slot() != nullptr);
}

building_item_slot::~building_item_slot()
{
}

void building_item_slot::create_item()
{
	assert_throw(this->get_item() == nullptr);

	static constexpr dice creation_chance_dice(1, 100);

	const int creation_chance_result = random::get()->roll_dice(creation_chance_dice);

	if (creation_chance_result <= 50) {
		return;
	}

	this->item = this->get_item_creation_type()->create_item();
	emit item_changed();
}

qunique_ptr<item> building_item_slot::take_item()
{
	assert_throw(this->get_item() != nullptr);

	auto item = std::move(this->item);
	emit item_changed();
	return item;
}

bool building_item_slot::can_buy_item(const metternich::character *buyer)
{
	if (this->get_item() == nullptr) {
		return false;
	}
	
	const int item_price = this->get_item()->get_price();

	if (buyer->get_game_data()->is_ruler()) {
		return buyer->get_game_data()->get_domain()->get_economy()->get_wealth() >= item_price;
	} else {
		//personal wealth for non-ruler characters not implemented yet
		return false;
	}
}

void building_item_slot::buy_item(const metternich::character *buyer)
{
	const int item_price = this->get_item()->get_price();

	if (buyer->get_game_data()->is_ruler()) {
		buyer->get_game_data()->get_domain()->get_economy()->change_wealth(-item_price);
	} else {
		//personal wealth for non-ruler characters not implemented yet
	}

	buyer->get_game_data()->add_item(this->take_item());
}


}
