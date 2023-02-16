#pragma once

namespace metternich {

class military_unit_type;

struct military_unit_type_compare final
{
	bool operator()(const military_unit_type *lhs, const military_unit_type *rhs) const;
};

using military_unit_type_set = std::set<const military_unit_type *, military_unit_type_compare>;

template <typename T>
using military_unit_type_map = std::map<const military_unit_type *, T, military_unit_type_compare>;

}
