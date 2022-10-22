#include "metternich.h"

#include "economy/resource_container.h"

#include "economy/resource.h"

namespace metternich {

bool resource_compare::operator()(const resource *resource, const metternich::resource *other_resource) const
{
	return resource->get_identifier() < other_resource->get_identifier();
}

}
