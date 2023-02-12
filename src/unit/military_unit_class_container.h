#pragma once

namespace metternich {

class military_unit_class;

struct military_unit_class_compare final
{
	bool operator()(const military_unit_class *lhs, const military_unit_class *rhs) const;
};

using military_unit_class_set = std::set<const military_unit_class *, military_unit_class_compare>;

template <typename T>
using military_unit_class_map = std::map<const military_unit_class *, T, military_unit_class_compare>;

}
