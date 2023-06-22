#include "metternich.h"

#include "map/route.h"

namespace metternich {

void route::check() const
{
	if (!this->get_color().isValid()) {
		throw std::runtime_error(std::format("Route \"{}\" has no color.", this->get_identifier()));
	}
}

}
