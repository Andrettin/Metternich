#pragma once

namespace metternich {

class population_type;

struct population_type_compare final
{
	bool operator()(const population_type *lhs, const population_type *rhs) const;
};

using population_type_set = std::set<const population_type *, population_type_compare>;

template <typename T>
using population_type_map = std::map<const population_type *, T, population_type_compare>;

}
