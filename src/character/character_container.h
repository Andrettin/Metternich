#pragma once

namespace metternich {

class character;

struct character_compare final
{
	bool operator()(const character *lhs, const character *rhs) const;
};

using character_set = std::set<const character *, character_compare>;

template <typename T>
using character_map = std::map<const character *, T, character_compare>;

}
