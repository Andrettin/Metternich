#pragma once

namespace metternich {

class population_class;

struct population_class_compare final
{
	bool operator()(const population_class *lhs, const population_class *rhs) const;
};

using population_class_set = std::set<const population_class *, population_class_compare>;

template <typename T>
using population_class_map = std::map<const population_class *, T, population_class_compare>;

}
