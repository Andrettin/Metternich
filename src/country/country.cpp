#include "metternich.h"

#include "country/country.h"

#include "country/country_type.h"
#include "util/assert_util.h"

namespace metternich {

country::country(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::minor_nation)
{
}

void country::check() const
{
	assert_throw(this->get_color().isValid());
}

}
