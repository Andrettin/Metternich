#pragma once

namespace metternich {

class consulate;

struct consulate_compare final
{
	bool operator()(const consulate *lhs, const consulate *rhs) const;
};

using consulate_set = std::set<const consulate *, consulate_compare>;

template <typename T>
using consulate_map = std::map<const consulate *, T, consulate_compare>;

}
