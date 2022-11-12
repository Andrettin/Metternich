#pragma once

namespace metternich {

class commodity;

struct commodity_compare final
{
	bool operator()(const commodity *lhs, const commodity *rhs) const;
};

using commodity_set = std::set<const commodity *, commodity_compare>;

template <typename T>
using commodity_map = std::map<const commodity *, T, commodity_compare>;

}
