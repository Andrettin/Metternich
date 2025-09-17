#pragma once

namespace metternich {

class law_group;

struct law_group_compare final
{
	bool operator()(const law_group *lhs, const law_group *rhs) const;
};

using law_group_set = std::set<const law_group *, law_group_compare>;

template <typename T>
using law_group_map = std::map<const law_group *, T, law_group_compare>;

}
