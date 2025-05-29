#include "metternich.h"

#include "species/phenotype_container.h"

#include "species/phenotype.h"

namespace metternich {

bool phenotype_compare::operator()(const phenotype *lhs, const phenotype *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

}
