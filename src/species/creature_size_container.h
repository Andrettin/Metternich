#pragma once

namespace metternich {

class creature_size;

struct creature_size_compare final
{
	bool operator()(const creature_size *lhs, const creature_size *rhs) const;
};

using creature_size_set = std::set<const creature_size *, creature_size_compare>;

template <typename T>
using creature_size_map = std::map<const creature_size *, T, creature_size_compare>;

}
