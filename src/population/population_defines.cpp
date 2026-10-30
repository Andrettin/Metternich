#include "metternich.h"

#include "population/population_defines.h"

#include "database/gsml_data.h"
#include "economy/food_type.h"
#include "script/factor.h"

namespace metternich {

population_defines::population_defines()
{
}

population_defines::~population_defines()
{
}

void population_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "population_food_consumption") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->population_food_consumption[magic_enum::enum_cast<food_type>(key).value()] = centesimal_int(value);
		});
	} else if (tag == "promotion_chance") {
		this->promotion_chance = std::make_unique<factor<population_unit>>();
		this->promotion_chance->process_gsml_data(scope);
	} else if (tag == "demotion_chance") {
		this->demotion_chance = std::make_unique<factor<population_unit>>();
		this->demotion_chance->process_gsml_data(scope);
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

void population_defines::check() const
{
	if (this->get_base_population_size() == 0) {
		throw std::runtime_error("No base population size has been defined.");
	}
}

}

#include "moc_population_defines.cpp"
