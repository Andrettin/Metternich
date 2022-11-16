#pragma once

namespace metternich {

class building_slot_type;

struct building_slot_type_compare final
{
	bool operator()(const building_slot_type *lhs, const building_slot_type *rhs) const;
};

using building_slot_type_set = std::set<const building_slot_type *, building_slot_type_compare>;

template <typename T>
using building_slot_type_map = std::map<const building_slot_type *, T, building_slot_type_compare>;

}
