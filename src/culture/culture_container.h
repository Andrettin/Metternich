#pragma once

namespace metternich {

class culture;

struct culture_compare final
{
	bool operator()(const culture *lhs, const culture *rhs) const;
};

using culture_set = std::set<const culture *, culture_compare>;

template <typename T>
using culture_map = std::map<const culture *, T, culture_compare>;

}
