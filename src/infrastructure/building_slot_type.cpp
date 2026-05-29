#include "metternich.h"

#include "infrastructure/building_slot_type.h"

#include "infrastructure/building_type.h"

namespace metternich {

void building_slot_type::check() const
{
	if (this->building_types_by_holding_type.empty()) {
		throw std::runtime_error(std::format("Building slot type \"{}\" has no buildings which can be built on it.", this->get_identifier()));
	}
}

void building_slot_type::add_building_type(const building_type *building_type)
{
	for (const holding_type *holding_type : building_type->get_holding_types()) {
		this->building_types_by_holding_type[holding_type].push_back(building_type);
	}
}

}
