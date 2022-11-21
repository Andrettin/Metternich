#pragma once

namespace metternich {

class phenotype;

struct phenotype_compare final
{
	bool operator()(const phenotype *lhs, const phenotype *rhs) const;
};

using phenotype_set = std::set<const phenotype *, phenotype_compare>;

template <typename T>
using phenotype_map = std::map<const phenotype *, T, phenotype_compare>;

}
