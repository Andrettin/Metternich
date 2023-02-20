#pragma once

namespace metternich {

class spell;

struct spell_compare final
{
	bool operator()(const spell *lhs, const spell *rhs) const;
};

using spell_set = std::set<const spell *, spell_compare>;

template <typename T>
using spell_map = std::map<const spell *, T, spell_compare>;

}
