#pragma once

namespace metternich {

class technology;

struct technology_compare final
{
	bool operator()(const technology *lhs, const technology *rhs) const;
};

using technology_set = std::set<const technology *, technology_compare>;

template <typename T>
using technology_map = std::map<const technology *, T, technology_compare>;

}
