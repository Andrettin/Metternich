#include "metternich.h"

#include "map/province_container.h"

#include "map/province.h"

namespace metternich {

bool province_compare::operator()(const province *province, const metternich::province *other_province) const
{
	return province->get_identifier() < other_province->get_identifier();
}

}
