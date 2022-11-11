#include "metternich.h"

#include "population/population_group_map.h"

#include "country/culture.h"
#include "population/population_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

population_group_key::population_group_key(const std::string &key_str)
{
	const std::vector<std::string> subkeys = string::split(key_str, '.');

	for (const std::string &subkey : subkeys) {
		if (this->type == nullptr) {
			this->type = population_type::try_get(subkey);
		} else if (this->culture == nullptr) {
			this->culture = culture::try_get(subkey);
		} else {
			assert_throw(false);
		}
	}
}

bool population_group_key::operator<(const population_group_key &rhs) const
{
	const int lhs_defined_property_count = this->get_defined_property_count();
	const int rhs_defined_property_count = rhs.get_defined_property_count();
	if (lhs_defined_property_count != rhs_defined_property_count) {
		return lhs_defined_property_count > rhs_defined_property_count;
	}

	if (this->type != rhs.type) {
		if (this->type == nullptr || rhs.type == nullptr) {
			return this->type != nullptr;
		}

		return this->type->get_identifier() < rhs.type->get_identifier();
	}

	if (this->culture != rhs.culture) {
		if (this->culture == nullptr || rhs.culture == nullptr) {
			return this->culture != nullptr;
		}

		return this->culture->get_identifier() < this->culture->get_identifier();
	}

	return false; //equal
}

}
