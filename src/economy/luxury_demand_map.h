#pragma once

namespace metternich {

class commodity;
enum class luxury_type;

using luxury_variant = std::variant<luxury_type, const commodity *>;

struct luxury_demand_compare final
{
	bool operator()(const luxury_variant &lhs, const luxury_variant &rhs) const;
};

template <typename T>
using luxury_demand_map = std::map<luxury_variant, T, luxury_demand_compare>;

}
