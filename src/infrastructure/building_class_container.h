#pragma once

namespace metternich {

class building_class;

struct building_class_compare final
{
	bool operator()(const building_class *lhs, const building_class *rhs) const;
};

using building_class_set = std::set<const building_class *, building_class_compare>;

template <typename T>
using building_class_map = std::map<const building_class *, T, building_class_compare>;

}
