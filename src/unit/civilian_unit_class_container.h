#pragma once

namespace metternich {

class civilian_unit_class;

struct civilian_unit_class_compare final
{
	bool operator()(const civilian_unit_class *lhs, const civilian_unit_class *rhs) const;
};

using civilian_unit_class_set = std::set<const civilian_unit_class *, civilian_unit_class_compare>;

template <typename T>
using civilian_unit_class_map = std::map<const civilian_unit_class *, T, civilian_unit_class_compare>;

}
