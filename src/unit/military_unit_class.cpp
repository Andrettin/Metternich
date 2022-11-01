#include "metternich.h"

#include "unit/military_unit_class.h"

#include "unit/military_unit_domain.h"
#include "util/vector_util.h"

namespace metternich {

military_unit_class::military_unit_class(const std::string &identifier)
	: named_data_entry(identifier), domain(military_unit_domain::land)
{
}

}
