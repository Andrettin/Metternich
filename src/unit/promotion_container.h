#pragma once

namespace metternich {

class promotion;

struct promotion_compare final
{
	bool operator()(const promotion *lhs, const promotion *rhs) const;
};

using promotion_set = std::set<const promotion *, promotion_compare>;

template <typename T>
using promotion_map = std::map<const promotion *, T, promotion_compare>;

}
