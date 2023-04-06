#pragma once

namespace metternich {

class advisor_type;

struct advisor_type_compare final
{
	bool operator()(const advisor_type *lhs, const advisor_type *rhs) const;
};

using advisor_type_set = std::set<const advisor_type *, advisor_type_compare>;

template <typename T>
using advisor_type_map = std::map<const advisor_type *, T, advisor_type_compare>;

}
