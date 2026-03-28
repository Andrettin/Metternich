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

std::map<item_key, std::vector<building_item_slot *>> building_item_slot::item_slots_to_map(const std::vector<building_item_slot *> &item_slots)
{
	std::map<item_key, std::vector<building_item_slot *>> item_slot_map;

	for (building_item_slot *item_slot : item_slots) {
		if (item_slot->get_item() == nullptr) {
			continue;
		}

		item_slot_map[item_slot->get_item()->to_item_key()].push_back(item_slot);
	}

	return item_slot_map;
}

building_item_slot::building_item_slot(const metternich::item_creation_type *item_creation_type, const metternich::building_slot *building_slot)
	: item_creation_type(item_creation_type), building_slot(building_slot)
{
	assert_throw(this->get_item_creation_type() != nullptr);
	assert_throw(this->get_building_slot() != nullptr);
}

building_item_slot::building_item_slot(const gsml_data &scope, const metternich::building_slot *building_slot)
	: building_item_slot(item_creation_type::get(scope.get_tag()), building_slot)
{
	scope.process(this);
}

building_item_slot::~building_item_slot()
{
}

void building_item_slot::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();

	throw std::runtime_error(std::format("Invalid building item slot property: \"{}\".", key));
}

void building_item_slot::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "item") {
		this->item = make_qunique<metternich::item>(scope);
	} else {
		throw std::runtime_error(std::format("Invalid building item slot scope: \"{}\".", tag));
	}
}

gsml_data building_item_slot::to_gsml_data() const
{
	gsml_data data(this->get_item_creation_type()->get_identifier());

	if (this->get_item() != nullptr) {
		data.add_child("item", this->get_item()->to_gsml_data());
	}

	return data;
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

	return buyer->get_game_data()->get_wealth() >= item_price;
}

void building_item_slot::buy_item(const metternich::character *buyer)
{
	const int item_price = this->get_item()->get_price();

	buyer->get_game_data()->change_wealth(-item_price);
	buyer->get_game_data()->add_item(this->take_item());
}

}
