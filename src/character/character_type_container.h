#pragma once

namespace metternich {

class character_type;

struct character_type_compare final
{
	bool operator()(const character_type *lhs, const character_type *rhs) const;
};

using character_type_set = std::set<const character_type *, character_type_compare>;

template <typename T>
using character_type_map = std::map<const character_type *, T, character_type_compare>;

}
