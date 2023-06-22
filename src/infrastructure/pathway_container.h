#pragma once

namespace metternich {

class pathway;

struct pathway_compare final
{
	bool operator()(const pathway *lhs, const pathway *rhs) const;
};

using pathway_set = std::set<const pathway *, pathway_compare>;

template <typename T>
using pathway_map = std::map<const pathway *, T, pathway_compare>;

}
