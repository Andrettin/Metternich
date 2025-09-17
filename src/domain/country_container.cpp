#include "metternich.h"

#include "domain/country_container.h"

#include "domain/country.h"

namespace metternich {

bool country_compare::operator()(const country *country, const metternich::country *other_country) const
{
	return country->get_identifier() < other_country->get_identifier();
}

}
