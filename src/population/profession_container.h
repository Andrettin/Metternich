#pragma once

namespace metternich {

class profession;

struct profession_compare final
{
	bool operator()(const profession *lhs, const profession *rhs) const;
};

using profession_set = std::set<const profession *, profession_compare>;

template <typename T>
using profession_map = std::map<const profession *, T, profession_compare>;

}
