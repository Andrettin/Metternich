#include "metternich.h"

#include "population/phenotype_container.h"

#include "population/phenotype.h"

namespace metternich {

bool phenotype_compare::operator()(const phenotype *lhs, const phenotype *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
