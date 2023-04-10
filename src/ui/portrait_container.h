#pragma once

namespace metternich {

class portrait;

struct portrait_compare final
{
	bool operator()(const portrait *lhs, const portrait *rhs) const;
};

using portrait_set = std::set<const portrait *, portrait_compare>;

template <typename T>
using portrait_map = std::map<const portrait *, T, portrait_compare>;

}
