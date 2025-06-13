#pragma once

namespace metternich {

class religion;

struct religion_compare final
{
	bool operator()(const religion *lhs, const religion *rhs) const;
};

using religion_set = std::set<const religion *, religion_compare>;

template <typename T>
using religion_map = std::map<const religion *, T, religion_compare>;

}
