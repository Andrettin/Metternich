#pragma once

namespace metternich {

class opinion_modifier;

struct opinion_modifier_compare final
{
	bool operator()(const opinion_modifier *lhs, const opinion_modifier *rhs) const;
};

template <typename T>
using opinion_modifier_map = std::map<const opinion_modifier *, T, opinion_modifier_compare>;

}
