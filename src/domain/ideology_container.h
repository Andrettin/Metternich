#pragma once

namespace metternich {

class ideology;

struct ideology_compare final
{
	bool operator()(const ideology *lhs, const ideology *rhs) const;
};

using ideology_set = std::set<const ideology *, ideology_compare>;

template <typename T>
using ideology_map = std::map<const ideology *, T, ideology_compare>;

}
