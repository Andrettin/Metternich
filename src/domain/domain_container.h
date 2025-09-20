#pragma once

namespace metternich {

class domain;

struct domain_compare final
{
	bool operator()(const domain *lhs, const domain *rhs) const;
};

using domain_set = std::set<const domain *, domain_compare>;

template <typename T>
using domain_map = std::map<const domain *, T, domain_compare>;

}
