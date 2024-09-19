#pragma once

namespace metternich {

class employment_type;

struct employment_type_compare final
{
	bool operator()(const employment_type *lhs, const employment_type *rhs) const;
};

using employment_type_set = std::set<const employment_type *, employment_type_compare>;

template <typename T>
using employment_type_map = std::map<const employment_type *, T, employment_type_compare>;

}
