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

bool item_class::is_weapon() const
{
	return this->get_slot()->is_weapon();
}

}
