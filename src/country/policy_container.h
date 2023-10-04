#pragma once

namespace metternich {

class policy;

struct policy_compare final
{
	bool operator()(const policy *lhs, const policy *rhs) const;
};

using policy_set = std::set<const policy *, policy_compare>;

template <typename T>
using policy_map = std::map<const policy *, T, policy_compare>;

}
