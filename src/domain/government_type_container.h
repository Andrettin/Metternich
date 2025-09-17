#pragma once

namespace metternich {

class government_type;

struct government_type_compare final
{
	bool operator()(const government_type *lhs, const government_type *rhs) const;
};

using government_type_set = std::set<const government_type *, government_type_compare>;

template <typename T>
using government_type_map = std::map<const government_type *, T, government_type_compare>;

}
