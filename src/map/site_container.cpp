#include "metternich.h"

#include "map/site_container.h"

#include "map/site.h"

namespace metternich {

bool site_compare::operator()(const site *site, const metternich::site *other_site) const
{
	return site->get_identifier() < other_site->get_identifier();
}

}
