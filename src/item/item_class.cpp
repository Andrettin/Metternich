#include "metternich.h"

#include "item/item_class.h"

#include "item/item_slot.h"

namespace metternich {

item_class::item_class(const std::string &identifier) : named_data_entry(identifier)
{
}

item_class::~item_class()
{
}

void item_class::check() const
{
	if (this->get_slot() != nullptr && this->is_consumable()) {
		throw std::runtime_error(std::format("Item class \"{}\" can both be equipped and consumed.", this->get_identifier()));
	}
}

bool item_class::is_weapon() const
{
	if (this->get_slot() != nullptr) {
		return this->get_slot()->is_weapon();
	}

	return false;
}

}
