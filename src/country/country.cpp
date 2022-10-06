#include "metternich.h"

#include "country/country.h"

#include "country/country_type.h"
#include "map/province.h"
#include "util/assert_util.h"

namespace metternich {

country::country(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::minor_nation)
{
}

void country::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "provinces") {
		for (const std::string &value : values) {
			this->provinces.push_back(province::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void country::check() const
{
	assert_throw(this->get_capital_province() != nullptr);
	assert_throw(this->get_color().isValid());
}

}
