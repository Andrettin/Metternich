#pragma once

namespace metternich {

class icon;

struct icon_compare final
{
	bool operator()(const icon *lhs, const icon *rhs) const;
};

using icon_set = std::set<const icon *, icon_compare>;

template <typename T>
using icon_map = std::map<const icon *, T, icon_compare>;

}
