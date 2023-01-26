#pragma once

namespace metternich {

class office;

struct office_compare final
{
	bool operator()(const office *lhs, const office *rhs) const;
};

using office_set = std::set<const office *, office_compare>;

template <typename T>
using office_map = std::map<const office *, T, office_compare>;

}
