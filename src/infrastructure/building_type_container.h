#pragma once

namespace metternich {

class building_type;

struct building_type_compare final
{
	bool operator()(const building_type *lhs, const building_type *rhs) const;
};

using building_type_set = std::set<const building_type *, building_type_compare>;

template <typename T>
using building_type_map = std::map<const building_type *, T, building_type_compare>;

}
