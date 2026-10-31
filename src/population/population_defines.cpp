#include "metternich.h"

#include "population/population_defines.h"

#include "database/gsml_data.h"
#include "economy/commodity.h"
#include "economy/food_type.h"
#include "script/factor.h"
#include "util/string_conversion_util.h"

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

			const food_type food_type = string::to_enum<metternich::food_type>(key);
			if (food_type == food_type::none) {
				throw std::runtime_error("A food type of none is not accepted for the population food consumption define.");
			}

			this->population_food_consumption[food_type] = decimillesimal_int(value);
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

	for (const auto &[food_type, consumption] : this->population_food_consumption) {
		if (!commodity::get_food_type_commodities().contains(food_type)) {
			throw std::runtime_error(std::format("Food type \"{}\" has consumption in the population food consumption define, but there are no commodities of that food type.", magic_enum::enum_name(food_type)));
		}
	}
}

}

#include "moc_population_defines.cpp"
