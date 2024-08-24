#pragma once

namespace metternich {

class tradition;

struct tradition_compare final
{
	bool operator()(const tradition *lhs, const tradition *rhs) const;
};

using tradition_set = std::set<const tradition *, tradition_compare>;

template <typename T>
using tradition_map = std::map<const tradition *, T, tradition_compare>;

}
