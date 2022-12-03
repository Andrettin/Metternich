#pragma once

namespace metternich {

class terrain_type;

struct terrain_type_compare final
{
	bool operator()(const terrain_type *lhs, const terrain_type *rhs) const;
};

using terrain_type_set = std::set<const terrain_type *, terrain_type_compare>;

template <typename T>
using terrain_type_map = std::map<const terrain_type *, T, terrain_type_compare>;

}
