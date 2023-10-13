#include "metternich.h"

#include "population/population_type_container.h"

#include "economy/commodity.h"
#include "population/population_type.h"

namespace metternich {

bool population_type_compare::operator()(const population_type *lhs, const population_type *rhs) const
{
	if ((lhs == nullptr || rhs == nullptr) && lhs != rhs) {
		return lhs == nullptr;
	}

	const commodity *lhs_output_commodity = lhs->get_output_commodity();
	const commodity *rhs_output_commodity = rhs->get_output_commodity();

	if (lhs_output_commodity != rhs_output_commodity) {
		if (lhs_output_commodity == nullptr || rhs_output_commodity == nullptr) {
			return lhs_output_commodity != nullptr;
		}

		if (lhs_output_commodity->is_labor() != rhs_output_commodity->is_labor()) {
			return lhs_output_commodity->is_labor();
		}

		if (lhs_output_commodity->is_storable() != rhs_output_commodity->is_storable()) {
			return !lhs_output_commodity->is_storable();
		}

		return lhs_output_commodity->get_identifier() < rhs_output_commodity->get_identifier();
	}

	if (lhs->get_output_value() != rhs->get_output_value()) {
		return lhs->get_output_value() < rhs->get_output_value();
	}

	return lhs->get_identifier() < rhs->get_identifier();
}

}
