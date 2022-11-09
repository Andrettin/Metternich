#include "metternich.h"

#include "country/culture_base.h"

#include "population/population_class.h"
#include "population/population_type.h"
#include "unit/civilian_unit_class.h"
#include "unit/civilian_unit_type.h"
#include "util/assert_util.h"

namespace metternich {

void culture_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "population_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const population_class *population_class = population_class::get(key);
			const population_type *population_type = population_type::get(value);
			this->set_population_class_type(population_class, population_type);
		});
	} else if (tag == "civilian_class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const civilian_unit_class *unit_class = civilian_unit_class::get(key);
			const civilian_unit_type *unit_type = civilian_unit_type::get(value);
			this->set_civilian_class_unit_type(unit_class, unit_type);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void culture_base::check() const
{
	for (const auto &[unit_class, unit_type] : this->civilian_class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}
}

}
