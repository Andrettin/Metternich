#pragma once

namespace metternich {

class route;

struct route_compare final
{
	bool operator()(const route *lhs, const route *rhs) const;
};

using route_set = std::set<const route *, route_compare>;

template <typename T>
using route_map = std::map<const route *, T, route_compare>;

}
