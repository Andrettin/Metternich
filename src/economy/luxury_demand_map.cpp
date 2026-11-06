#include "metternich.h"

#include "economy/luxury_demand_map.h"

#include "economy/commodity.h"

namespace metternich {

bool luxury_demand_compare::operator()(const luxury_variant &lhs, const luxury_variant &rhs) const
{
	if (lhs.index() != rhs.index()) {
		return lhs.index() < rhs.index();
	}

	if (std::holds_alternative<const commodity *>(lhs)) {
		return std::get<const commodity *>(lhs)->get_index() < std::get<const commodity *>(rhs)->get_index();
	} else {
		return std::get<luxury_type>(lhs) < std::get<luxury_type>(rhs);
	}
}

}
