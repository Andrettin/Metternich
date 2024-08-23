#pragma once

namespace metternich {

class improvement;

struct improvement_compare final
{
	bool operator()(const improvement *lhs, const improvement *rhs) const;
};

using improvement_set = std::set<const improvement *, improvement_compare>;

template <typename T>
using improvement_map = std::map<const improvement *, T, improvement_compare>;

}
