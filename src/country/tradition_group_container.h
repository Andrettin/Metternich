#pragma once

namespace metternich {

class tradition_group;

struct tradition_group_compare final
{
	bool operator()(const tradition_group *lhs, const tradition_group *rhs) const;
};

using tradition_group_set = std::set<const tradition_group *, tradition_group_compare>;

template <typename T>
using tradition_group_map = std::map<const tradition_group *, T, tradition_group_compare>;

}
