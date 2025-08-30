#pragma once

namespace metternich {

class character_class;

struct character_class_compare final
{
	bool operator()(const character_class *lhs, const character_class *rhs) const;
};

using character_class_set = std::set<const character_class *, character_class_compare>;

template <typename T>
using character_class_map = std::map<const character_class *, T, character_class_compare>;

}
